#include <iostream>
#include <oci.h>
#include <cstring> // Fixed: provides strlen

int main() {
    OCIEnv *envhp;
    OCIError *errhp;
    OCISvcCtx *svchp;
    OCIStmt *stmthp;
    OCIDefine *defnp = (OCIDefine *)0;
    char version_buffer[100] = {0}; // Initialize to empty

    std::cout << "Starting connection to XEPDB1..." << std::endl;

    // 1. Setup & Login
    OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, NULL);
    
    sword login_status = OCILogon(envhp, errhp, &svchp, 
                                 (unsigned char*)"PRAKRITI", 8, 
                                 (unsigned char*)"Prakriti076", 11, 
                                 (unsigned char*)"localhost:1521/XEPDB1", 21);

    if (login_status != OCI_SUCCESS) {
        std::cout << "Login failed!" << std::endl;
        return 1;
    }

    // 2. Prepare and Execute SQL
    OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
    const char* sql = "SELECT banner FROM v$version WHERE ROWNUM = 1"; // More reliable version query
    OCIStmtPrepare(stmthp, errhp, (unsigned char*)sql, (ub4)strlen(sql), OCI_NTV_SYNTAX, OCI_DEFAULT);

    // 3. Define Output
    OCIDefineByPos(stmthp, &defnp, errhp, 1, (void *)version_buffer, 100, SQLT_STR, NULL, NULL, NULL, OCI_DEFAULT);

    // 4. Execute and Fetch
    std::cout << "Executing query..." << std::endl;
    if (OCIStmtExecute(svchp, stmthp, errhp, 1, 0, NULL, NULL, OCI_DEFAULT) == OCI_SUCCESS) {
        std::cout << "Database Full Info: " << version_buffer << std::endl;
    } else {
        std::cout << "Query execution failed." << std::endl;
    }

    // Cleanup
    OCILogoff(svchp, errhp);
    OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    OCIHandleFree(errhp, OCI_HTYPE_ERROR);
    OCIHandleFree(envhp, OCI_HTYPE_ENV);
    
    return 0;
}