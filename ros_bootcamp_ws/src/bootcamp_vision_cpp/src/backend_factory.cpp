#include "bootcamp_vision_cpp/inference_backend.hpp"
#include "bootcamp_vision_cpp/opencv_dnn_backend.hpp"
#include <iostream>
#include <memory>

namespace bootcamp_vision_cpp
{

std::unique_ptr<InferenceBackend> create_backend(const std::string& backend_name)
{
  // Always use OpenCV DNN
  if (backend_name == "auto" || backend_name == "opencv") {
    std::cout << "[Backend] Using: OpenCV DNN" << std::endl;
    return std::make_unique<OpenCVDNNBackend>();
  }
  
  std::cerr << "[Backend] Unknown backend: " << backend_name << std::endl;
  std::cerr << "[Backend] Falling back to OpenCV DNN" << std::endl;
  return std::make_unique<OpenCVDNNBackend>();
}

std::vector<std::string> get_available_backends()
{
  return {"opencv"};
}

}  // namespace bootcamp_vision_cpp
