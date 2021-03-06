# amelethdf/src/lib CMakeList.txt file

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${AMELETHDF_DEP_INCLUDE_DIRS})

FILE(GLOB_RECURSE amelethdf_src *.c)
ADD_DEFINITIONS(-DAMELETHDF_C_LIBRARY)
ADD_LIBRARY(amelethdfc SHARED ${amelethdf_src})


IF (WIN32)
  SET_TARGET_PROPERTIES(amelethdfc PROPERTIES 
    OUTPUT_NAME "amelethdfc")
ELSE ()
  ADD_LIBRARY(amelethdfc_static STATIC ${amelethdf_src})
  SET_TARGET_PROPERTIES(amelethdfc amelethdfc_static PROPERTIES 
    OUTPUT_NAME "amelethdfc")
ENDIF ()


  
TARGET_LINK_LIBRARIES(amelethdfc ${AMELETHDF_DEP_LINK_LIBS})

INSTALL(TARGETS amelethdfc 
  EXPORT  AmeletHDFLibraryDepends
  RUNTIME       DESTINATION "${INSTALL_BIN_DIR}"
  LIBRARY       DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE       DESTINATION "${INSTALL_LIB_DIR}"
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}"
  )
IF (UNIX)
  INSTALL(TARGETS amelethdfc_static
    EXPORT  AmeletHDFLibraryDepends
    RUNTIME       DESTINATION "${INSTALL_BIN_DIR}"
    LIBRARY       DESTINATION "${INSTALL_LIB_DIR}"
    ARCHIVE       DESTINATION "${INSTALL_LIB_DIR}"
    PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}"
    )
ENDIF()

FILE(GLOB_RECURSE amelethdf_header *.h)
INSTALL(FILES ${amelethdf_header} DESTINATION "include")

add_static_analysis(amelethdfc)