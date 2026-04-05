#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

string multiplyStrings(string num1, string num2) {
    int n = num1.size(), m = num2.size();
    if (num1 == "0" || num2 == "0") return "0";
    
    // Result can have at most n + m digits
    vector<int> result(n + m, 0);
    
    // Reverse the strings to process from least significant digit
    reverse(num1.begin(), num1.end());
    reverse(num2.begin(), num2.end());
    
    // Multiply each digit
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            int mul = (num1[i] - '0') * (num2[j] - '0');
            result[i + j] += mul;
        }
    }
    
    // Handle carry-over
    for (int i = 0; i < n + m - 1; i++) {
        result[i + 1] += result[i] / 10;
        result[i] %= 10;
    }
    
    // Convert result array to string
    string res = "";
    for (int i = n + m - 1; i >= 0; i--) {
        res += to_string(result[i]);
    }
    
    // Remove leading zeros
    size_t pos = res.find_first_not_of('0');
    return (pos == string::npos) ? "0" : res.substr(pos);
}

int main() {
    string num1 = "123456789";
    string num2 = "987654321";
    
    cout << num1 << " × " << num2 << " = " << multiplyStrings(num1, num2) << endl;
    
    return 0;
}
