# Install service
# TODO: make script more Intelligent
install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/sysDetector.service
        ${CMAKE_CURRENT_LIST_DIR}/sysDetector-proc.service
    PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ
        GROUP_EXECUTE GROUP_READ
        WORLD_READ WORLD_EXECUTE
    DESTINATION
        ${SYSDETECTOR_SERVICE_DIR}
)
