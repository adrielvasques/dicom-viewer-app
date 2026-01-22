cmake_minimum_required(VERSION 3.16)
project(dicom-visualizer VERSION 1.0.0 LANGUAGES CXX)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(DICOM_INSTALL_ON_BUILD "Run install step after building" OFF)

if(DICOM_INSTALL_ON_BUILD AND UNIX)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix" FORCE)
    endif()
endif()

# Qt6 Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg OpenGL OpenGLWidgets)

# Find DCMTK
find_package(DCMTK REQUIRED)

# Define source files
set(CORE_SOURCES
    src/core/CDicomLoader.cpp
    src/core/CDicomImage.cpp
    src/core/CDicomMetadata.cpp
)

set(INFRASTRUCTURE_SOURCES
    src/infrastructure/dcmtk/DcmtkDicomLoader.cpp
    src/infrastructure/qt/QtImageRenderer.cpp
    src/infrastructure/qt/QtReportGenerator.cpp
)

set(PRESENTATION_SOURCES
    src/presentation/viewmodels/MainViewModel.cpp
)

set(UI_SOURCES
    src/ui/CMainWindow.cpp
    src/ui/CImageViewer.cpp
    src/ui/CMetadataPanel.cpp
    src/ui/CThumbnailWidget.cpp
)

set(UTIL_SOURCES
    src/utils/CImageConverter.cpp
    src/utils/CColorPalette.cpp
)

set(HEADERS
    src/core/CDicomLoader.h
    src/core/CDicomImage.h
    src/core/CDicomMetadata.h
    src/application/ports/IDicomLoader.h
    src/application/ports/IImageRenderer.h
    src/application/ports/IReportGenerator.h
    src/application/dto/ReportData.h
    src/infrastructure/dcmtk/DcmtkDicomLoader.h
    src/infrastructure/qt/QtImageRenderer.h
    src/infrastructure/qt/QtReportGenerator.h
    src/presentation/viewmodels/MainViewModel.h
    src/ui/CMainWindow.h
    src/ui/CImageViewer.h
    src/ui/CMetadataPanel.h
    src/ui/CThumbnailWidget.h
    src/utils/CImageConverter.h
    src/utils/CColorPalette.h
    include/DicomViewer/Types.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
qt_add_executable(dicom-visualizer
    src/main.cpp
    ${CORE_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${PRESENTATION_SOURCES}
    ${UI_SOURCES}
    ${UTIL_SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Include directories
target_include_directories(dicom-visualizer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${DCMTK_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(dicom-visualizer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ${DCMTK_LIBRARIES}
    dcmjpeg
    ijg8
    ijg12
    ijg16
)

# Platform-specific settings
if(WIN32)
    set_target_properties(dicom-visualizer PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install rules (Linux desktop integration)
install(TARGETS dicom-visualizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES dicom-visualizer.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

if(DICOM_INSTALL_ON_BUILD)
    add_custom_command(TARGET dicom-visualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
                --config $<CONFIG>
                --prefix ${CMAKE_INSTALL_PREFIX}
        COMMENT "Installing dicom-visualizer to ${CMAKE_INSTALL_PREFIX}"
        VERBATIM
    )
endif()
