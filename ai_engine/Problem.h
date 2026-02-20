#pragma once
#include <string>

class Problem {
private:
    std::string statement;
    std::string constraints;

public:
    // Constructor
    Problem(std::string stmt, std::string cons) {
        statement = stmt;
        constraints = cons;
    }

    // Getters
    std::string getStatement() const { 
        return statement; 
    }
    
    std::string getConstraints() const { 
        return constraints; 
    }
};