#include "async_http.h"

#ifdef USE_ESP32

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include <esp_crt_bundle.h>
#endif

namespace esphome {
namespace async_http {

static const char *const TAG = "async_http";

void AsyncHttpComponent::setup() {
  // Run on core 0 next to the network stack, the main loop runs on core 1.
  // Not subscribed to the task watchdog, so a slow request can't reset the device.
  BaseType_t res = xTaskCreatePinnedToCore(AsyncHttpComponent::task_func_, "async_http", this->task_stack_size_, this,
                                           1, &this->task_handle_, 0);
  if (res != pdPASS) {
    ESP_LOGE(TAG, "Failed to create task");
    this->mark_failed();
    return;
  }
  this->disable_loop();
}

void AsyncHttpComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Async HTTP:");
  ESP_LOGCONFIG(TAG, "  Timeout: %" PRIu32 "ms", this->timeout_);
  ESP_LOGCONFIG(TAG, "  Keep-Alive Timeout: %" PRIu32 "ms", this->keep_alive_timeout_);
  ESP_LOGCONFIG(TAG, "  Buffer Size RX: %u, TX: %u", this->buffer_size_rx_, this->buffer_size_tx_);
  ESP_LOGCONFIG(TAG, "  Max Response Size: %u", (unsigned) this->max_response_size_);
  ESP_LOGCONFIG(TAG, "  Task Stack Size: %" PRIu32, this->task_stack_size_);
}

void AsyncHttpComponent::get(std::string url, std::vector<Header> headers) {
  if (this->is_failed())
    return;

  if (this->state_.load() != STATE_IDLE) {
    // The running request may be for outdated parameters, send this one after it
    ESP_LOGD(TAG, "Request already running, queueing %s", url.c_str());
    this->pending_url_ = std::move(url);
    this->pending_headers_ = std::move(headers);
    this->has_pending_ = true;
    return;
  }

  this->url_ = std::move(url);
  this->headers_ = std::move(headers);
  this->start_();
}

void AsyncHttpComponent::start_() {
  this->state_.store(STATE_BUSY);
  xTaskNotifyGive(this->task_handle_);
}

void AsyncHttpComponent::loop() {
  if (this->state_.load() != STATE_DONE)
    return;

  if (this->has_pending_) {
    ESP_LOGD(TAG, "Discarding response of %s, a newer request is queued", this->url_.c_str());
    this->has_pending_ = false;
    this->url_ = std::move(this->pending_url_);
    this->headers_ = std::move(this->pending_headers_);
    this->body_.clear();
    this->start_();
    return;
  }

  // Move everything out, so the triggers may queue a new request
  std::string body = std::move(this->body_);
  this->body_ = std::string();
  int status_code = this->status_code_;
  esp_err_t error = this->error_;
  this->state_.store(STATE_IDLE);
  this->disable_loop();

  if (error != ESP_OK) {
    ESP_LOGW(TAG, "Request to %s failed after %" PRIu32 "ms: %s", this->url_.c_str(), this->duration_ms_,
             esp_err_to_name(error));
    this->status_momentary_warning("failed", 1000);
    this->error_trigger_.trigger();
    return;
  }

  ESP_LOGD(TAG, "Received response from %s: status %d, %u bytes in %" PRIu32 "ms", this->url_.c_str(), status_code,
           (unsigned) body.size(), this->duration_ms_);
  if (this->truncated_) {
    ESP_LOGW(TAG, "Response was larger than max_response_size (%u bytes) and got truncated",
             (unsigned) this->max_response_size_);
  }
  this->response_trigger_.trigger(status_code, body);
}

void AsyncHttpComponent::task_func_(void *arg) {
  auto *self = static_cast<AsyncHttpComponent *>(arg);
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (self->state_.load() != STATE_BUSY)
      continue;

    self->perform_();
    ESP_LOGV(TAG, "Task stack high water mark: %u bytes", (unsigned) uxTaskGetStackHighWaterMark(nullptr));

    self->state_.store(STATE_DONE);
    self->enable_loop_soon_any_context();
  }
}

esp_err_t AsyncHttpComponent::http_event_handler_(esp_http_client_event_t *evt) {
  if (evt->event_id != HTTP_EVENT_ON_DATA)
    return ESP_OK;

  auto *self = static_cast<AsyncHttpComponent *>(evt->user_data);
  // Skip the body of redirect responses, the client follows them on its own
  int status = esp_http_client_get_status_code(evt->client);
  if (status >= 300 && status < 400)
    return ESP_OK;

  size_t room = self->max_response_size_ - self->body_.size();
  size_t len = evt->data_len;
  if (len > room) {
    len = room;
    self->truncated_ = true;
  }
  self->body_.append(static_cast<const char *>(evt->data), len);
  return ESP_OK;
}

// Runs in the worker task
void AsyncHttpComponent::perform_() {
  this->status_code_ = 0;
  this->error_ = ESP_OK;
  const uint32_t start = millis();

  // Drop a connection that sat idle for too long, the server has most likely closed it already
  if (this->connected_ && start - this->last_used_ > this->keep_alive_timeout_) {
    ESP_LOGV(TAG, "Closing idle connection");
    esp_http_client_close(this->client_);
    this->connected_ = false;
  }

  if (this->client_ == nullptr) {
    esp_http_client_config_t config = {};
    config.url = this->url_.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = this->timeout_;
    config.buffer_size = this->buffer_size_rx_;
    config.buffer_size_tx = this->buffer_size_tx_;
    config.event_handler = AsyncHttpComponent::http_event_handler_;
    config.user_data = this;
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    config.crt_bundle_attach = esp_crt_bundle_attach;
#endif
    this->client_ = esp_http_client_init(&config);
    if (this->client_ == nullptr) {
      this->error_ = ESP_FAIL;
      this->duration_ms_ = millis() - start;
      return;
    }
  } else {
    // Keeps the open connection if the host stays the same
    esp_http_client_set_url(this->client_, this->url_.c_str());
  }

  for (const auto &header : this->headers_) {
    esp_http_client_set_header(this->client_, header.first.c_str(), header.second.c_str());
  }

  this->body_.clear();
  this->truncated_ = false;
  this->error_ = esp_http_client_perform(this->client_);

  if (this->error_ != ESP_OK && this->connected_) {
    // The server may have closed the kept-alive connection, try again on a new one
    ESP_LOGV(TAG, "Request on reused connection failed (%s), reconnecting", esp_err_to_name(this->error_));
    esp_http_client_close(this->client_);
    this->body_.clear();
    this->truncated_ = false;
    this->error_ = esp_http_client_perform(this->client_);
  }

  if (this->error_ == ESP_OK) {
    this->status_code_ = esp_http_client_get_status_code(this->client_);
  }

  if (this->error_ == ESP_OK && this->keep_alive_timeout_ > 0) {
    this->connected_ = true;
  } else {
    esp_http_client_close(this->client_);
    this->connected_ = false;
  }

  this->last_used_ = millis();
  this->duration_ms_ = this->last_used_ - start;
}

}  // namespace async_http
}  // namespace esphome

#endif  // USE_ESP32
