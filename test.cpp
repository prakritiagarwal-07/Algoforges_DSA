#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <oci.h>

// Capture web response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Robust slug parser
std::vector<std::string> extractSlugs(const std::string& rawJson) {
    std::vector<std::string> slugs;
    std::string key = "\"titleSlug\":\"";
    size_t pos = rawJson.find(key);
    while (pos != std::string::npos) {
        size_t start = pos + key.length();
        size_t end = rawJson.find("\"", start);
        if (end != std::string::npos) {
            slugs.push_back(rawJson.substr(start, end - start));
        }
        pos = rawJson.find(key, end);
    }
    return slugs;
}

int main() {
    std::cout << "--- AlgoForge: Stealth Ingestion Mode ---" << std::endl;

    int skipValue = 0;
    std::cout << "Enter the number of questions to SKIP (e.g., 200): ";
    std::cin >> skipValue;

    // 1. FETCH MASTER LIST
    std::string masterListJson;
    CURL* curl = curl_easy_init();
    if(curl) {
        std::string mUrl = "https://alfa-leetcode-api.onrender.com/problems?limit=100&skip=" + std::to_string(skipValue);
        curl_easy_setopt(curl, CURLOPT_URL, mUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &masterListJson);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    // Safety Check: Did we get blocked at Step 1?
    if (masterListJson.find("Too many request") != std::string::npos) {
        std::cout << "!! BLOCK DETECTED: API says try again in 1 hour. Stopping." << std::endl;
        return 1;
    }

    std::vector<std::string> allSlugs = extractSlugs(masterListJson);
    if (allSlugs.empty()) {
        std::cout << "!! No slugs found. Check your skip value." << std::endl;
        return 1;
    }

    // 2. ORACLE CONNECTION
    OCIEnv *envhp; OCIError *errhp; OCISvcCtx *svchp;
    OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, NULL);
    sword status = OCILogon(envhp, errhp, &svchp, (unsigned char*)"PRAKRITI", 8, (unsigned char*)"Prakriti076", 11, (unsigned char*)"localhost:1521/XEPDB1", 21);
    
    if (status != OCI_SUCCESS) {
        std::cout << "Oracle Connection Failed!" << std::endl;
        return 1;
    }

    // 3. MAIN LOOP WITH SAFETY GATE
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delayDist(20, 40); // Slower delay for safety

    for (size_t i = 0; i < allSlugs.size(); ++i) {
        std::string currentSlug = allSlugs[i];
        std::string problemData;

        // Fetch Individual Question
        curl = curl_easy_init();
        if(curl) {
            std::string qUrl = "https://alfa-leetcode-api.onrender.com/select?titleSlug=" + currentSlug;
            curl_easy_setopt(curl, CURLOPT_URL, qUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0 Safari/537.36");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &problemData);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        // --- SAFETY GATE ---
        if (problemData.find("Too many request") != std::string::npos) {
            std::cout << "\n!! CRITICAL: IP Blocked during loop. Stopping to save your DB." << std::endl;
            break; 
        }

        // --- ORACLE INSERT ---
        OCIStmt *stmthp;
        std::string sql = "INSERT INTO LEETCODE_PROBLEMS (TITLE, SLUG, RAW_JSON) VALUES (:t, :s, :j)";
        OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
        OCIStmtPrepare(stmthp, errhp, (unsigned char*)sql.c_str(), sql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT);
        
        OCIBind *b1=NULL, *b2=NULL, *b3=NULL;
        OCIBindByName(stmthp, &b1, errhp, (unsigned char*)":t", -1, (void*)currentSlug.c_str(), currentSlug.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);
        OCIBindByName(stmthp, &b2, errhp, (unsigned char*)":s", -1, (void*)currentSlug.c_str(), currentSlug.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);
        OCIBindByName(stmthp, &b3, errhp, (unsigned char*)":j", -1, (void*)problemData.c_str(), problemData.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);

        OCIStmtExecute(svchp, stmthp, errhp, 1, 0, NULL, NULL, OCI_COMMIT_ON_SUCCESS);

        std::cout << "Stored (" << (i+1) << "/" << allSlugs.size() << ") [" << currentSlug << "] - " << (allSlugs.size() - (i+1)) << " left." << std::endl;

        // COUNTDOWN
        if (i < allSlugs.size() - 1) {
            int waitTime = delayDist(gen);
            std::cout << "Next in: " << std::flush;
            for (int c = waitTime; c > 0; --c) {
                std::cout << c << "... " << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "GO!" << std::endl;
        }
    }

    std::cout << "Session Complete." << std::endl;
    return 0;
}