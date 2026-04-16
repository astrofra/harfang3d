# Generates the Harfang API XML description used to generate the documentation.

set(HG_API_XML "${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml")

add_custom_command(
	OUTPUT
		${HG_API_XML}
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_SOURCE_DIR}/binding/bind_harfang.py --xml --out ${CMAKE_CURRENT_BINARY_DIR}/harfang ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_SOURCE_DIR}/binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang API description file")

add_custom_target(gen_api_xml ALL DEPENDS ${HG_API_XML})

if(HG_BUILD_STATIC_DOCS)
	file(GLOB HG_DOC_MARKDOWN_FILES CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/doc/*.md
		${CMAKE_CURRENT_SOURCE_DIR}/doc/tree_desc.txt)
	file(GLOB HG_DOC_UTILITY_FILES CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/doc_utils/*.py
		${CMAKE_CURRENT_SOURCE_DIR}/doc_to_static_html.py
		${CMAKE_CURRENT_SOURCE_DIR}/markdown.css)
	file(GLOB HG_DOC_IMAGE_FILES CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/img/*)

	set(HG_STATIC_DOC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/static_html")
	set(HG_STATIC_DOC_STAMP "${CMAKE_CURRENT_BINARY_DIR}/static_html.stamp")

	add_custom_command(
		OUTPUT
			${HG_STATIC_DOC_STAMP}
		COMMAND
			${CMAKE_COMMAND} -E remove_directory ${HG_STATIC_DOC_OUTPUT}
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${HG_STATIC_DOC_OUTPUT}
		COMMAND
			${Python3_EXECUTABLE}
				${CMAKE_CURRENT_SOURCE_DIR}/doc_to_static_html.py
				--api ${HG_API_XML}
				--doc ${CMAKE_CURRENT_SOURCE_DIR}/doc
				--img ${CMAKE_CURRENT_SOURCE_DIR}/img
				--css ${CMAKE_CURRENT_SOURCE_DIR}/markdown.css
				--out ${HG_STATIC_DOC_OUTPUT}
				--version ${HG_VERSION}
				--project-name Harfang
		COMMAND
			${CMAKE_COMMAND} -E touch ${HG_STATIC_DOC_STAMP}
		DEPENDS
			${HG_API_XML}
			${HG_DOC_MARKDOWN_FILES}
			${HG_DOC_UTILITY_FILES}
			${HG_DOC_IMAGE_FILES}
		WORKING_DIRECTORY
			${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
		COMMENT
			"Generating Harfang static HTML documentation")

	add_custom_target(harfang_static_docs
		DEPENDS ${HG_STATIC_DOC_STAMP})
	add_dependencies(harfang_static_docs gen_api_xml)

	add_custom_target(harfang_static_docs_install
		COMMAND
			${CMAKE_COMMAND} -E remove_directory ${CMAKE_INSTALL_PREFIX}/docs/harfang
		COMMAND
			${CMAKE_COMMAND} -E copy_directory ${HG_STATIC_DOC_OUTPUT} ${CMAKE_INSTALL_PREFIX}/docs/harfang
		DEPENDS
			harfang_static_docs
		VERBATIM)
endif()

# add_custom_command(
# 	OUTPUT
# 		${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml
# 	COMMAND
# 		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../binding/bind_harfang.py --xml --out ${CMAKE_CURRENT_BINARY_DIR}/harfang ${HG_BINDING_DEFINES}
# 	MAIN_DEPENDENCY
# 		${CMAKE_SOURCE_DIR}/binding/bind_harfang.py
# 	WORKING_DIRECTORY
# 		${HG_FABGEN_PATH}
# 	COMMENT
# 		"Generating Harfang API description file")

# offline docs
#configure_file(doc/index.html.in ${CMAKE_INSTALL_PREFIX}/offline_docs/index.html @ONLY IMMEDIATE)
#add_custom_target(offline_docs ALL
#		${Python3_EXECUTABLE} doc_to_html.py --project_name Harfang --doc_path doc --api_path ${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml --out_path ${CMAKE_INSTALL_PREFIX}/offline_docs --version ${HG_VERSION}
#	WORKING_DIRECTORY
#		${CMAKE_CURRENT_SOURCE_DIR}
#	DEPENDS 
#		${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml)
#install(DIRECTORY img DESTINATION offline_docs/content COMPONENT offline_docs)
#set_target_properties(offline_docs PROPERTIES FOLDER "harfang/doc")
