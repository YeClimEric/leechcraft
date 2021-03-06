cmake_minimum_required (VERSION 2.8)
project (leechcraft_hotstreams)
include (InitLCPlugin OPTIONAL)

if (NOT USE_QT5)
	find_package (QJSON REQUIRED)
else ()
	unset (QJSON_LIBRARIES)
	unset (QJSON_INCLUDE_DIR)
endif ()

include_directories (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	${QJSON_INCLUDE_DIR}
	)
set (SRCS
	hotstreams.cpp
	icecastfetcher.cpp
	icecastmodel.cpp
	radiostation.cpp
	somafmlistfetcher.cpp
	stealkilllistfetcher.cpp
	streamlistfetcherbase.cpp
	stringlistradiostation.cpp
	)

if (QJSON_FOUND OR USE_QT5)
	add_definitions (-DHAVE_QJSON)
	set (SRCS
		${SRCS}
		audioaddictstreamfetcher.cpp
		rockradiolistfetcher.cpp)
endif ()

set (RESOURCES hotstreamsresources.qrc)
QtAddResources (RCCS ${RESOURCES})

CreateTrs("hotstreams" "en;ru_RU" COMPILED_TRANSLATIONS)
CreateTrsUpTarget("hotstreams" "en;ru_RU" "${SRCS}" "" "")

add_library (leechcraft_hotstreams SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	${RCCS}
	)
target_link_libraries (leechcraft_hotstreams
	${LEECHCRAFT_LIBRARIES}
	${QJSON_LIBRARIES}
	)
install (TARGETS leechcraft_hotstreams DESTINATION ${LC_PLUGINS_DEST})

FindQtLibs (leechcraft_hotstreams Concurrent Gui Network Xml)
