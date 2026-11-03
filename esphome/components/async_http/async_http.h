#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#ifdef USE_ESP32

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esphome {
namespace async_http {

using Header = std::pair<std::string, std::string>;

/// Performs HTTP GET requests in a separate FreeRTOS task, so the DNS lookup,
/// TLS handshake and download never block the main loop (or trip its watchdog).
/// The connection is kept alive between requests, so the TLS handshake only
/// happens when it was idle longer than keep_alive_timeout.
/// The response triggers still run on the main loop.
class AsyncHttpComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_timeout(uint32_t timeout) { this->timeout_ = timeout; }
  void set_keep_alive_timeout(uint32_t timeout) { this->keep_alive_timeout_ = timeout; }
  void set_buffer_size_rx(uint16_t size) { this->buffer_size_rx_ = size; }
  void set_buffer_size_tx(uint16_t size) { this->buffer_size_tx_ = size; }
  void set_max_response_size(size_t size) { this->max_response_size_ = size; }
  void set_task_stack_size(uint32_t size) { this->task_stack_size_ = size; }

  /// Queue a GET request. If one is already running, its response is discarded
  /// and this request is sent right after it finishes.
  void get(std::string url, std::vector<Header> headers);

  bool is_busy() const { return this->state_.load() != STATE_IDLE; }

  Trigger<int, std::string &> *get_response_trigger() { return &this->response_trigger_; }
  Trigger<> *get_error_trigger() { return &this->error_trigger_; }

 protected:
  enum State : uint8_t { STATE_IDLE, STATE_BUSY, STATE_DONE };

  static void task_func_(void *arg);
  static esp_err_t http_event_handler_(esp_http_client_event_t *evt);
  void start_();
  void perform_();

  uint32_t timeout_{5000};
  uint32_t keep_alive_timeout_{50000};
  uint16_t buffer_size_rx_{2048};
  uint16_t buffer_size_tx_{1024};
  size_t max_response_size_{8192};
  uint32_t task_stack_size_{10240};

  TaskHandle_t task_handle_{nullptr};
  // Worker task only: the client is kept between requests to reuse the TLS connection
  esp_http_client_handle_t client_{nullptr};
  bool connected_{false};
  uint32_t last_used_{0};
  std::atomic<uint8_t> state_{STATE_IDLE};

  // Owned by the worker task while state_ is STATE_BUSY, by the main loop otherwise
  std::string url_;
  std::vector<Header> headers_;
  std::string body_;
  int status_code_{0};
  esp_err_t error_{ESP_OK};
  bool truncated_{false};
  uint32_t duration_ms_{0};

  // Main loop only
  bool has_pending_{false};
  std::string pending_url_;
  std::vector<Header> pending_headers_;

  Trigger<int, std::string &> response_trigger_;
  Trigger<> error_trigger_;
};

template<typename... Ts> class AsyncHttpGetAction : public Action<Ts...> {
 public:
  explicit AsyncHttpGetAction(AsyncHttpComponent *parent) : parent_(parent) {}

  void set_url(TemplatableValue<std::string, Ts...> url) { this->url_ = url; }
  void add_request_header(const char *key, TemplatableValue<std::string, Ts...> value) {
    this->request_headers_.emplace_back(key, value);
  }

  void play(const Ts &...x) override {
    std::vector<Header> headers;
    headers.reserve(this->request_headers_.size());
    for (const auto &header : this->request_headers_) {
      headers.emplace_back(header.first, header.second.value(x...));
    }
    this->parent_->get(this->url_.value(x...), std::move(headers));
  }

 protected:
  AsyncHttpComponent *parent_;
  TemplatableValue<std::string, Ts...> url_{};
  std::vector<std::pair<const char *, TemplatableValue<std::string, Ts...>>> request_headers_{};
};

}  // namespace async_http
}  // namespace esphome

#endif  // USE_ESP32
