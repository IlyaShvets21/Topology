set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools Concurrent)
set(QT_VERSION_MAJOR 5)

add_executable(Topology
	main.cpp
	MainWindow.cpp
	MainWindow.hpp
	Logger.cpp
	Logger.hpp
)

terget_link_libraries(LogWindoeApp PRIVATE Qt5::Widgets)