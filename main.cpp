#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

int main() {
    vector<int> vec;
    cout << "Hello, World!" << endl;
    for (int i = 10; i > 0; i--){

        vec.push_back(i);
    }
    sort(vec.begin(), vec.end());

    for (int j = 0; j < 10; j++) {

        cout << "the sorted vector is: " << vec.at(j) << endl;
    }

    return 0;
}