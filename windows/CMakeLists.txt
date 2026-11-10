cmake_minimum_required(VERSION 3.21)

project(caelestia-shell-windows VERSION 0.2.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools QuickEffects Svg)
qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(caelestia-shell
    src/main.cpp
    src/platform/platform.cpp
    src/platform/platform.hpp
    src/platform/services/audioservice.cpp
    src/platform/services/audioservice.hpp
    src/platform/services/batteryservice.cpp
    src/platform/services/batteryservice.hpp
    src/platform/services/bluetoothservice.cpp
    src/platform/services/bluetoothservice.hpp
    src/platform/services/networkservice.cpp
    src/platform/services/networkservice.hpp
    src/platform/services/trayservice.cpp
    src/platform/services/trayservice.hpp
)

target_link_libraries(caelestia-shell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickEffects
    Qt6::Svg
    dwmapi
    shell32
    user32
    wlanapi
    iphlpapi
    ws2_32
    ole32
    uuid
    propsys
    shcore
    windowsapp
)

target_compile_definitions(caelestia-shell PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

# C++/WinRT headers (Windows.Devices.Radios) are shipped in the Windows SDK
# under Include/<version>/cppwinrt, which is not part of the default MSVC
# include path.
file(GLOB CPPWINRT_HEADERS
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsSDKVersion}cppwinrt/winrt/Windows.Devices.Radios.h"
    "C:/Program Files (x86)/Windows Kits/10/Include/*/cppwinrt/winrt/Windows.Devices.Radios.h"
)
if(CPPWINRT_HEADERS)
    list(GET CPPWINRT_HEADERS -1 CPPWINRT_HEADER)
    get_filename_component(CPPWINRT_INCLUDE_DIR "${CPPWINRT_HEADER}" DIRECTORY)
    get_filename_component(CPPWINRT_INCLUDE_DIR "${CPPWINRT_INCLUDE_DIR}" DIRECTORY)
    target_include_directories(caelestia-shell PRIVATE "${CPPWINRT_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "C++/WinRT headers not found in the Windows SDK")
endif()

target_include_directories(caelestia-shell PRIVATE
    third_party/m3shapes/src
    third_party/m3shapes/src/qml
)

qt_add_qml_module(caelestia-shell
    URI Caelestia.Shell
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/Theme.qml
        src/qml/Anim.qml
        src/qml/CAnim.qml
        src/qml/PanelController.qml
        src/qml/Bar.qml
        src/qml/components/StyledText.qml
        src/qml/components/StyledRect.qml
        src/qml/components/StateLayer.qml
        src/qml/components/Logo.qml
        src/qml/components/MaterialIcon.qml
        src/qml/components/RoundedMask.qml
        src/qml/components/RoundedClip.qml
        src/qml/components/PanelSurface.qml
        src/qml/components/M3Switch.qml
        src/qml/components/M3Slider.qml
        src/qml/components/M3Button.qml
        src/qml/components/M3ListItem.qml
        src/qml/bar/BarWorkspaces.qml
        src/qml/bar/BarActiveWindow.qml
        src/qml/bar/BarTray.qml
        src/qml/bar/BarClock.qml
        src/qml/bar/BarStatusIcons.qml
        src/qml/bar/BarPower.qml
        src/qml/overlay/OverlayWindow.qml
        src/qml/overlay/ScreenSurface.qml
        src/qml/overlay/SpringDeform.qml
        src/qml/overlay/SidebarShell.qml
        src/qml/overlay/DashboardShell.qml
        src/qml/overlay/PanelHost.qml
        src/qml/overlay/FoundationProbe.qml
        src/qml/popouts/PopoutFrame.qml
    SOURCES
        third_party/m3shapes/src/core/Cubic.cpp
        third_party/m3shapes/src/core/Cubic.hpp
        third_party/m3shapes/src/core/Feature.cpp
        third_party/m3shapes/src/core/Feature.hpp
        third_party/m3shapes/src/core/Point.hpp
        third_party/m3shapes/src/core/CornerRounding.hpp
        third_party/m3shapes/src/core/RoundedPolygon.cpp
        third_party/m3shapes/src/core/RoundedPolygon.hpp
        third_party/m3shapes/src/core/Utils.hpp
        third_party/m3shapes/src/morph/FeatureMapping.cpp
        third_party/m3shapes/src/morph/FeatureMapping.hpp
        third_party/m3shapes/src/morph/FloatMapping.cpp
        third_party/m3shapes/src/morph/FloatMapping.hpp
        third_party/m3shapes/src/morph/Morph.cpp
        third_party/m3shapes/src/morph/Morph.hpp
        third_party/m3shapes/src/morph/PolygonMeasure.cpp
        third_party/m3shapes/src/morph/PolygonMeasure.hpp
        third_party/m3shapes/src/qml/MaterialShapeItem.cpp
        third_party/m3shapes/src/qml/MaterialShapeItem.hpp
        third_party/m3shapes/src/qml/SmoothShapeMaterial.cpp
        third_party/m3shapes/src/qml/SmoothShapeMaterial.hpp
        third_party/m3shapes/src/shapes/MaterialShapes.cpp
        third_party/m3shapes/src/shapes/MaterialShapes.hpp
        third_party/m3shapes/src/shapes/Shapes.cpp
        third_party/m3shapes/src/shapes/Shapes.hpp
)

qt_add_resources(caelestia-shell "assets"
    PREFIX "/"
    FILES
        assets/wallpaper.webp
        assets/wallpaper.png
        assets/logo.svg
        assets/fonts/GoogleSansFlex.ttf
        assets/fonts/MaterialSymbolsRounded.ttf
        assets/fonts/Rubik-VariableFont_wght.ttf
        assets/fonts/CaskaydiaCoveNerdFont-Regular.ttf
        assets/fonts/LICENSE-GoogleSansFlex.txt
        assets/fonts/LICENSE-MaterialSymbols.txt
        assets/fonts/LICENSE-Rubik.txt
        assets/fonts/LICENSE-CaskaydiaCove.txt
)

# Qt 6's QSB emits HLSL alongside GLSL. The original project uses NOHLSL,
# which makes its shaders unusable by the default Direct3D 11 RHI backend.
qt_add_shaders(caelestia-shell "shaders"
    PRECOMPILE
    GLSL "300es,330"
    HLSL "50"
    FILES
        src/shaders/roundedmask.vert
        src/shaders/roundedmask.frag
        src/shaders/blob.vert
        src/shaders/blob.frag
)

# M3Shapes' analytic-antialiasing shaders. Resource prefix must stay
# :/m3shapes/shaders because SmoothShapeMaterial loads that path directly.
qt_add_shaders(caelestia-shell "m3shapes_shaders"
    PREFIX "/m3shapes"
    BASE "third_party/m3shapes/src"
    GLSL "300es,330"
    HLSL "50"
    FILES
        third_party/m3shapes/src/shaders/smoothshape.vert
        third_party/m3shapes/src/shaders/smoothshape.frag
)
