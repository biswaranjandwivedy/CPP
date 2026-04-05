#include <iostream>
#include <string>
#include <stack>
using namespace std;

bool isBalanced(string s) {
    stack<char> st;
    
    for (char ch : s) {
        if (ch == '(' || ch == '[' || ch == '{') {
            st.push(ch);
        } 
        else if (ch == ')' || ch == ']' || ch == '}') {
            if (st.empty()) return false;
            
            char top = st.top();
            st.pop();
            
            if ((ch == ')' && top != '(') ||
                (ch == ']' && top != '[') ||
                (ch == '}' && top != '{')) {
                return false;
            }
        }
    }
    
    return st.empty();
}

int main() {
    vector<string> testCases = {
        "({[]})",      // true
        "({[}])",      // false
        "((()))",      // true
        "({[",         // false
        ")}]",         // false
        ""             // true
    };
    
    for (const string& test : testCases) {
        cout << "\"" << test << "\" -> " 
             << (isBalanced(test) ? "Balanced" : "Not Balanced") << endl;
    }
    
    return 0;
}
