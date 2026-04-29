file(GLOB_RECURSE ASSET_FILES
  "${ASSET_DIR}/*.png"
  "${ASSET_DIR}/*.jpg"
  "${ASSET_DIR}/*.jpeg"
  "${ASSET_DIR}/*.hdr"
  "${ASSET_DIR}/*.mtl"
  "${ASSET_DIR}/*.obj"
  "${ASSET_DIR}/*.gltf"
  "${ASSET_DIR}/*.bin"
)

foreach(f IN LISTS ASSET_FILES)
  file(RELATIVE_PATH rel "${ASSET_DIR}" "${f}")
  get_filename_component(dir "${rel}" DIRECTORY)
  if(dir)
    file(MAKE_DIRECTORY "${BUILD_DIR}/${dir}")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${f}" "${BUILD_DIR}/${rel}"
  )
endforeach()
