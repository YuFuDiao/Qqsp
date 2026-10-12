cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(qqsp VERSION 1.10.0)
include(FeatureSummary)
include(GNUInstallDirs)
include(CPack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES off)

# UTF-8 Chinese literals: MSVC must not reinterpret them as ANSI.
if (MSVC)
    add_compile_options(/utf-8)
endif()

set(MINIMAL_QT_VERSION 6.7)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

find_package(Qt6 ${MINIMAL_QT_VERSION} COMPONENTS LinguistTools WebEngineCore WebEngineWidgets)
find_package(Qt6 ${MINIMAL_QT_VERSION} REQUIRED COMPONENTS Core Gui Multimedia MultimediaWidgets Widgets)
find_package(Qsp REQUIRED)
if (WIN32)
	find_package(Oniguruma REQUIRED) # to install the library
endif()

option(NO_NATIVE_DIALOGS "Do not use native file open/sav dialogs" OFF)
option(BUILD_DEBUGGER "Build tools to debug games" ON)
configure_file(qqsp-config.h.cmakein qqsp-config.h @ONLY)

qt_standard_project_setup(
	REQUIRES ${MINIMAL_QT_VERSION}
	I18N_TRANSLATED_LANGUAGES ru uk
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_add_executable(Qqsp WIN32)
target_sources(Qqsp PRIVATE
	callbacks_gui.cpp
	callbacks_gui.h
	comtools.cpp
	comtools.h
	main.cpp
	mainwindow.cpp
	mainwindow.h
	optionsdialog.cpp
	optionsdialog.h
	qspimgcanvas.cpp
	qspimgcanvas.h
	qspinputbox.cpp
	qspinputbox.h
	qspinputdlg.cpp
	qspinputdlg.h
	qsplistbox.cpp
	qsplistbox.h
	qspmsgdlg.cpp
	qspmsgdlg.h
	qspstr.cpp
	qspstr.h
	qsptextbox.cpp
	qsptextbox.h
	videolabel.cpp
	videolabel.h
	mainwindow.ui
	optionsdialog.ui
	qqsp.qrc
)

if (WIN32)
	target_sources(Qqsp PRIVATE Qqsp.rc)
endif()

target_link_libraries(Qqsp PRIVATE Qsp::qsp Qt::Multimedia Qt::MultimediaWidgets Qt::Widgets)

if(Qt6WebEngineWidgets_FOUND)
	target_sources(Qqsp PRIVATE
		qspwebbox.cpp
		qspwebengineurlrequestinterceptor.cpp
		qspwebengineurlschemehandler.cpp
		qspexecwebengineurlschemehandler.cpp
		url_schemes.cpp
		qspwebchannel.cpp
		qspwebbox.h
		qspwebengineurlrequestinterceptor.h
		qspwebengineurlschemehandler.h
		qspexecwebengineurlschemehandler.h
		url_schemes.h
		qspwebchannel.h
		webinspector.h
		webinspector.cpp
		webinspectorwindow.h
		webinspectorwindow.cpp
		webinspectorwindow.ui
	)

	target_link_libraries(Qqsp PRIVATE Qt::WebEngineCore Qt::WebEngineWidgets)
endif()

if (Qt6LinguistTools_FOUND)
	qt_add_translations(Qqsp
		TS_FILES
			Qqsp.en.ts Qqsp.ru.ts Qqsp.uk.ts
			qt_uk.ts qtbase_uk.ts
	)
endif()

if (BUILD_DEBUGGER)
	add_subdirectory(debugger)
endif()

# On Windows we usually do not have system-wide Qt install, let's make the built
# binaries able to find Qt
if (WIN32)
	# let's find out where Qt is installed
	get_target_property(_qtpaths_executable Qt::qtpaths LOCATION)
	execute_process(COMMAND "${_qtpaths_executable}" "--query"
		OUTPUT_VARIABLE qt_paths
	)
    string(REGEX MATCH "QT_INSTALL_PREFIX:([^\n]+)" _qt_install_prefix "${qt_paths}")
	string(REGEX MATCH "QT_INSTALL_PLUGINS:([^\n]+)" _qt_install_plugins "${qt_paths}")
	string(REPLACE "QT_INSTALL_PREFIX:" "" QT_INSTALL_PREFIX "${_qt_install_prefix}")
	string(REPLACE "QT_INSTALL_PLUGINS:" "" QT_INSTALL_PLUGINS "${_qt_install_plugins}")

	# Windows 8.1
	add_compile_definitions(WINVER=0x0603 _WIN32_WINNT=0x0603)

	# get Qt installation prefix
	cmake_path(RELATIVE_PATH QT_INSTALL_PLUGINS BASE_DIRECTORY "${QT_INSTALL_PREFIX}"  OUTPUT_VARIABLE QT_PLUGINS_SUBDIR)
	configure_file(cmake/qt.conf.cmakein "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" @ONLY)

	target_link_libraries(Qqsp PRIVATE oniguruma::onig)
	add_custom_command(TARGET Qqsp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Qqsp> "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" $<TARGET_FILE_DIR:Qqsp>
		COMMAND_EXPAND_LISTS
	)
endif()

install(TARGETS Qqsp DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES Qqsp.zh_CN.qm DESTINATION "${CMAKE_INSTALL_BINDIR}")
if (LINUX)
	install(FILES Qqsp.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
	install(FILES qsp.mime DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/mime/packages" RENAME qsp.xml)
else()
	if(APPLE)
		set(executable_path "$<TARGET_FILE_NAME:Qqsp>.app")
	else()
		set(executable_path "\${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:Qqsp>")
	endif()

	install(IMPORTED_RUNTIME_ARTIFACTS Qsp::qsp DESTINATION "${CMAKE_INSTALL_BINDIR}")
	install(IMPORTED_RUNTIME_ARTIFACTS oniguruma::onig DESTINATION "${CMAKE_INSTALL_BINDIR}")
	qt_generate_deploy_script(
		TARGET Qqsp
		OUTPUT_SCRIPT qt_deploy_script
		CONTENT "
	qt_deploy_runtime_dependencies(
		EXECUTABLE \"${executable_path}\"
		GENERATE_QT_CONF
		VERBOSE
	)")

    install(SCRIPT ${qt_deploy_script})
endif()

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)
