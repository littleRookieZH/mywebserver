#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int min_moves_to_end(vector<int> &move) {
    int n = move.size();
    int steps = 0;
    int left = 0, right = n - 1;

    while (left < right) {
        int max_jump = move[left];
        int next_position = left;
        for (int i = 1; i <= max_jump; ++i) {
            if (left + i >= n - 1) {
                next_position = n - 1;
                break;
            }
            if (move[left + i] >= max_jump - i) {
                next_position = left + i;
                break;
            }
        }
        if (next_position == left) {
            return -1; // 无法到达终点
        }
        left = next_position;
        ++steps;

        if (left >= right)
            break;

        max_jump = move[right];
        next_position = right;
        for (int i = 1; i <= max_jump; ++i) {
            if (right - i <= left) {
                next_position = left;
                break;
            }
            if (move[right - i] >= max_jump - i) {
                next_position = right - i;
                break;
            }
        }
        if (next_position == right) {
            return -1; // 无法到达终点
        }
        right = next_position;
        // ++steps;
    }

    return steps;
}

int main() {
    vector<int> move = {4, 3, 0, 1, 4};

    int steps = min_moves_to_end(move);
    if (steps != -1) {
        cout << "最少步数：" << steps << endl;
    } else {
        cout << "无法到达终点" << endl;
    }

    return 0;
}
