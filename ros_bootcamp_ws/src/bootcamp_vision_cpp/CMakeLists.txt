cmake_minimum_required(VERSION 3.8)
project(bootcamp_vision_cpp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate service interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/EnrollFace.srv"
  "srv/ListFaces.srv"
  "srv/DeleteFace.srv"
  DEPENDENCIES std_msgs
)

# Define USE_OPENCV_DNN for conditional compilation
add_definitions(-DUSE_OPENCV_DNN)

# Camera publisher node

if(USE_ONNXRUNTIME)
  # Use build directory for ONNXRuntime installation
  set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/onnxruntime")
  
  # Check if we need to build ONNXRuntime with CUDA
  if(BUILD_ONNXRUNTIME_CUDA AND NOT EXISTS "${ONNXRUNTIME_DIR}/lib/libonnxruntime.so")
    message(STATUS "========================================")
    message(STATUS "Building ONNXRuntime with CUDA support")
    message(STATUS "This will take 30-60 minutes...")
    message(STATUS "========================================")
    
    # Detect CUDA installation location
    # Check for standard CUDA toolkit location first
    if(EXISTS "/usr/local/cuda")
      set(CUDA_HOME_PATH "/usr/local/cuda")
    # Check for system-wide installation (common in Docker containers)
    elseif(EXISTS "/usr/include/cuda.h")
      set(CUDA_HOME_PATH "/usr")
    else()
      message(FATAL_ERROR "CUDA not found. Please install CUDA toolkit.")
    endif()
    
    message(STATUS "Using CUDA from: ${CUDA_HOME_PATH}")
    
    # Select ONNXRuntime version based on CMake version
    # v1.23.2 requires CMake 3.28+, v1.19.0 requires 3.26+, v1.16.0 works with CMake 3.22+
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28.0")
      set(ONNXRUNTIME_VERSION "v1.23.2")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.26.0")
      set(ONNXRUNTIME_VERSION "v1.19.0")
    else()
      set(ONNXRUNTIME_VERSION "v1.16.0")
      message(STATUS "Using ONNXRuntime ${ONNXRUNTIME_VERSION} for CMake ${CMAKE_VERSION}")
    endif()
    
    include(ExternalProject)
    
    # Build ONNXRuntime with CUDA in build directory
    ExternalProject_Add(
      onnxruntime_build
      GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
      GIT_TAG ${ONNXRUNTIME_VERSION}
      GIT_SHALLOW TRUE
      SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/onnxruntime-src"
      UPDATE_COMMAND ""
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/onnxruntime-src/build.sh
        --config Release
        --build_shared_lib
        --parallel
        --compile_no_warning_as_error
        --use_cuda
        --cuda_home ${CUDA_HOME_PATH}
        --cudnn_home /usr/lib/x86_64-linux-gnu
        --skip_tests
        --skip_submodule_sync
        --cmake_extra_defines
          CMAKE_INSTALL_PREFIX=${ONNXRUNTIME_DIR}
          onnxruntime_BUILD_UNIT_TESTS=OFF
          onnxruntime_BUILD_SHARED_LIB=ON
      INSTALL_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/onnxruntime-src/build/Linux/Release && make install
      BUILD_IN_SOURCE TRUE
    )
    
    # Make sure vision_node depends on ONNXRuntime build
    set(ONNXRUNTIME_BUILD_DEPENDENCY onnxruntime_build)
  endif()
  
  if(EXISTS "${ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h")
    message(STATUS "Using ONNXRuntime from ${ONNXRUNTIME_DIR}")
    set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_DIR}/include")
    set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_DIR}/lib")
    
    # Check if CUDA-enabled
    if(EXISTS "${ONNXRUNTIME_DIR}/lib/libonnxruntime_providers_cuda.so")
      message(STATUS "  ONNXRuntime CUDA provider: FOUND")
      set(ONNXRUNTIME_HAS_CUDA TRUE)
    else()
      message(STATUS "  ONNXRuntime CUDA provider: NOT FOUND (CPU-only)")
      message(STATUS "  To enable GPU: colcon build --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON")
      set(ONNXRUNTIME_HAS_CUDA FALSE)
    endif()
    
    # Always build both backends - selection at runtime
    add_compile_definitions(USE_ONNXRUNTIME)
    add_compile_definitions(USE_OPENCV_DNN)
  else()
    if(BUILD_ONNXRUNTIME_CUDA)
      message(STATUS "ONNXRuntime will be built during compilation...")
      # Enable both backends
      add_compile_definitions(USE_ONNXRUNTIME)
      add_compile_definitions(USE_OPENCV_DNN)
    else()
      message(WARNING "ONNXRuntime not found. Options:")
      message(WARNING "  1. Run: scripts/setup_onnxruntime.sh")
      message(WARNING "  2. Build with CUDA: colcon build --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON")
      message(WARNING "Using OpenCV DNN only")
      set(USE_ONNXRUNTIME OFF)
      add_compile_definitions(USE_OPENCV_DNN)
    endif()
  endif()
else()
  # OpenCV DNN only
  add_compile_definitions(USE_OPENCV_DNN)
endif()

# Set model directory in build space (not source!)
set(MODEL_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/models")
set(MODEL_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/models")
set(MODEL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/models")

# Download YOLO model during build (optional - controlled by environment variable)
option(DOWNLOAD_YOLO_MODELS "Download YOLO models during build" ON)

# Check if model already exists in source directory (from manual setup)
if(EXISTS "${MODEL_SOURCE_DIR}/tinyyolov2.onnx")
  message(STATUS "TinyYOLOv2 model found in source directory: ${MODEL_SOURCE_DIR}/tinyyolov2.onnx")
  message(STATUS "  Will install existing model (skipping download)")
  
  # Install the existing model from source
  install(FILES "${MODEL_SOURCE_DIR}/tinyyolov2.onnx"
          DESTINATION share/${PROJECT_NAME}/models)
  
  # Also check for face detector models
  if(NOT EXISTS "${MODEL_SOURCE_DIR}/face_detector.caffemodel")
    message(STATUS "Face detector not found in source, will download...")
    
    # Download face detector even though YOLO model exists
    file(MAKE_DIRECTORY "${MODEL_BUILD_DIR}")
    add_custom_command(
      OUTPUT "${MODEL_BUILD_DIR}/face_detector.caffemodel"
      COMMAND ${CMAKE_COMMAND} -E echo "Downloading face detector model (ResNet-SSD)..."
      COMMAND curl -L -o "${MODEL_BUILD_DIR}/face_detector.caffemodel"
              "https://github.com/opencv/opencv_3rdparty/raw/dnn_samples_face_detector_20170830/res10_300x300_ssd_iter_140000.caffemodel"
      COMMAND curl -L -o "${MODEL_BUILD_DIR}/face_detector.prototxt"
              "https://raw.githubusercontent.com/opencv/opencv/master/samples/dnn/face_detector/deploy.prototxt"
      COMMENT "Downloading OpenCV face detector models..."
      VERBATIM
    )
    
    add_custom_target(download_face_models ALL
      DEPENDS "${MODEL_BUILD_DIR}/face_detector.caffemodel"
    )
    
    install(FILES 
            "${MODEL_BUILD_DIR}/face_detector.caffemodel"
            "${MODEL_BUILD_DIR}/face_detector.prototxt"
            DESTINATION share/${PROJECT_NAME}/models
            OPTIONAL)
  else()
    message(STATUS "Face detector found in source directory")
    install(FILES 
            "${MODEL_SOURCE_DIR}/face_detector.caffemodel"
            "${MODEL_SOURCE_DIR}/face_detector.prototxt"
            DESTINATION share/${PROJECT_NAME}/models)
  endif()
  
  # Also check for YOLOv8 if it exists (for ONNXRuntime backend)
  if(EXISTS "${MODEL_SOURCE_DIR}/yolov8n.onnx")
    message(STATUS "YOLOv8n model also found in source directory")
    install(FILES "${MODEL_SOURCE_DIR}/yolov8n.onnx"
            DESTINATION share/${PROJECT_NAME}/models)
  endif()
  
  # Also install face models if they exist
  if(EXISTS "${MODEL_SOURCE_DIR}/det_10g.onnx" AND EXISTS "${MODEL_SOURCE_DIR}/w600k_r50.onnx")
    message(STATUS "Face models found in source directory")
    install(FILES 
            "${MODEL_SOURCE_DIR}/det_10g.onnx"
            "${MODEL_SOURCE_DIR}/w600k_r50.onnx"
            DESTINATION share/${PROJECT_NAME}/models)
  endif()

elseif(DOWNLOAD_YOLO_MODELS)
  # Model not in source, try to download during build
  message(STATUS "TinyYOLOv2 model will be downloaded during build.")
  message(STATUS "  Build directory: ${MODEL_BUILD_DIR}")
  message(STATUS "  Install directory: ${MODEL_INSTALL_DIR}")
  
  # Create build models directory
  file(MAKE_DIRECTORY "${MODEL_BUILD_DIR}")
  
  # Custom command to download TinyYOLOv2 model (runs during build)
  add_custom_command(
    OUTPUT "${MODEL_BUILD_DIR}/tinyyolov2.onnx"
    COMMAND ${CMAKE_COMMAND} -E echo "Downloading TinyYOLOv2 model (compatible with OpenCV DNN 4.6.0)..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODEL_BUILD_DIR}"
    COMMAND curl -L -o "${MODEL_BUILD_DIR}/tinyyolov2.onnx"
            "https://github.com/onnx/models/raw/main/validated/vision/object_detection_segmentation/tiny-yolov2/model/tinyyolov2-8.onnx"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Downloading TinyYOLOv2 model from ONNX Model Zoo..."
    VERBATIM
  )
  
  # Create a custom target that depends on all model files
  add_custom_target(yolo_model ALL
    DEPENDS "${MODEL_BUILD_DIR}/tinyyolov2.onnx"
  )
  
  # Install the downloaded models
  install(FILES "${MODEL_BUILD_DIR}/tinyyolov2.onnx"
          DESTINATION share/${PROJECT_NAME}/models
          OPTIONAL)  # Optional in case download fails
  
  # Download face detector model (ResNet-SSD Caffe model)
  add_custom_command(
    OUTPUT "${MODEL_BUILD_DIR}/face_detector.caffemodel"
    COMMAND ${CMAKE_COMMAND} -E echo "Downloading face detector model (ResNet-SSD)..."
    COMMAND curl -L -o "${MODEL_BUILD_DIR}/face_detector.caffemodel"
            "https://github.com/opencv/opencv_3rdparty/raw/dnn_samples_face_detector_20170830/res10_300x300_ssd_iter_140000.caffemodel"
    COMMAND curl -L -o "${MODEL_BUILD_DIR}/face_detector.prototxt"
            "https://raw.githubusercontent.com/opencv/opencv/master/samples/dnn/face_detector/deploy.prototxt"
    COMMENT "Downloading OpenCV face detector models..."
    VERBATIM
  )
  
  # Update yolo_model target to depend on face models too
  add_custom_target(download_models ALL
    DEPENDS "${MODEL_BUILD_DIR}/tinyyolov2.onnx" "${MODEL_BUILD_DIR}/face_detector.caffemodel"
  )
  
  # Install face detector models
  install(FILES 
          "${MODEL_BUILD_DIR}/face_detector.caffemodel"
          "${MODEL_BUILD_DIR}/face_detector.prototxt"
          DESTINATION share/${PROJECT_NAME}/models
          OPTIONAL)
  
else()
  message(STATUS "Model auto-download disabled (DOWNLOAD_YOLO_MODELS=OFF)")
  message(WARNING "No model found. Run scripts/setup_models.sh to download manually.")
endif()

# Create empty models directory for manual installation
install(DIRECTORY
  DESTINATION share/${PROJECT_NAME}/models
)

# Camera publisher node
add_executable(camera_pub_node src/camera_pub_node.cpp)
ament_target_dependencies(camera_pub_node 
  rclcpp 
  sensor_msgs 
  cv_bridge
)
target_link_libraries(camera_pub_node ${OpenCV_LIBS})

# Vision node with backend abstraction
add_executable(vision_node 
  src/vision_node.cpp
  src/backend_factory.cpp
  src/opencv_dnn_backend.cpp
)

ament_target_dependencies(vision_node 
  rclcpp 
  sensor_msgs 
  std_msgs
  cv_bridge
  ament_index_cpp
)

# Link services to vision_node
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(vision_node "${cpp_typesupport_target}")

# Add include directory for headers
target_include_directories(vision_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vision_node ${OpenCV_LIBS})

if(USE_ONNXRUNTIME)
  # Add dependency on ONNXRuntime build if needed
  if(DEFINED ONNXRUNTIME_BUILD_DEPENDENCY)
    add_dependencies(vision_node ${ONNXRUNTIME_BUILD_DEPENDENCY})
  endif()
  
  target_include_directories(vision_node PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
  target_link_directories(vision_node PRIVATE ${ONNXRUNTIME_LIB_DIR})
  target_link_libraries(vision_node onnxruntime)
  
  # Set RPATH so the executable can find libonnxruntime.so at runtime
  set_target_properties(vision_node PROPERTIES
    INSTALL_RPATH "${ONNXRUNTIME_LIB_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  
  message(STATUS "vision_node built with ONNXRuntime backend support")
else()
  message(STATUS "vision_node built without ONNXRuntime backend")
endif()

message(STATUS "vision_node built with OpenCV DNN backend support")

# Install executables
install(TARGETS
  camera_pub_node
  vision_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

# Install scripts
install(PROGRAMS
  scripts/download_yolo_models.py
  scripts/setup_models.sh
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
