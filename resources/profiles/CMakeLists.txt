include_directories(
"${CMAKE_SOURCE_DIR}/scribus"
)

install(FILES
GenericCMYK.icm
GenericCMYK.txt
ISOcoated_v2_300_bas.icc
ISOcoated_v2_300_bas.txt
Readme
sRGB.icm
srgb.license
  DESTINATION ${SHAREDIR}profiles
)
