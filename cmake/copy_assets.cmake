file(GLOB_RECURSE ASSET_FILES
  "${ASSET_DIR}/*.png"
  "${ASSET_DIR}/*.mtl"
  "${ASSET_DIR}/*.obj"
)

foreach(f IN LISTS ASSET_FILES)
  get_filename_component(name "${f}" NAME)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${f}" "${BUILD_DIR}/${name}"
  )
endforeach()
