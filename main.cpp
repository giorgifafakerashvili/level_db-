#include <iostream>
#include <memory>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <climits>
#include "include/slice.hpp"
#include "include/cache.hpp"
#include "include/status.hpp"
#include "include/comparator.hpp"
#include "include/iterator.hpp"

template<typename Dtype, typename Comparator = std::less<Dtype>>
class BST {
private:
    struct Node {
        Dtype data;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;

        Node() {}

        Node(const Dtype& data)
                : data(data) {}
    };
    Comparator comp;
    std::shared_ptr<Node> root_;

public:
    BST() {}
    BST(const Dtype& data)
            : root_(std::make_shared<Node>(data)) {}

    void Insert(const Dtype& data) {
        if(root_ == nullptr) {
            root_ = std::make_shared<Node>(data);
        } else {
            Insert(root_, data);
        }
    }

    void Inorder() {
        Inorder(root_);
    }

    bool IsSame(const BST<Dtype, Comparator>& other) {
        return IsSame(root_, other.root_);
    }

private:
    std::shared_ptr<Node> Insert(std::shared_ptr<Node>& root, const Dtype& data) {
        if(root == nullptr) {
            return std::make_shared<Node>(data);
        }

        if(comp(data, root->data)) {
            root->left = Insert(root->left, data);
        } else if(root->data, data) {
            root->right = Insert(root->right, data);
        } else {
            //
        }

        return root;
    }

    bool IsSame(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
        if(a == nullptr) return b == nullptr;
        if(b == nullptr) return a == nullptr;

        return (a->data == b->data && IsSame(a->left, b->left) && IsSame(a->right, b->right));
    }

    void Inorder(const std::shared_ptr<Node>& root) {
        if(root != nullptr) {
            Inorder(root->left);
            std::cout << root->data << " ";
            Inorder(root->right);
        }
    }
};

/*
TEST(IsSameTestBST, EmptyBsts) {
    BST<int> a,b;
    EXPECT_TRUE(a.IsSame(b));
}

TEST(IsSameTestBST, TrivialCases) {
    BST<int> a,b;
    a.Insert(10);
    b.Insert(10);

    EXPECT_TRUE(a.IsSame(b));

    a.Insert(5);
    b.Insert(5);

    EXPECT_TRUE(a.IsSame(b));

    a.Insert(7);
    b.Insert(7);
    a.Insert(14);
    b.Insert(14);

    EXPECT_TRUE(a.IsSame(b));
}

TEST(IsSameTESTBST, NotSameCases) {
    BST<int> a, b;
    a.Insert(10);

    EXPECT_FALSE(a.IsSame(b));

    b.Insert(5);
    EXPECT_FALSE(a.IsSame(b));

    a.Insert(7);
    b.Insert(7);
    EXPECT_FALSE(a.IsSame(b));
}

TEST(IsSameTESTBST, SameValuesDifferentConfigurations) {
    BST<int> a, b;
    a.Insert(1);
    a.Insert(2);

    b.Insert(2);
    b.Insert(1);

    EXPECT_FALSE(a.IsSame(b));
}
*/


void OptionalBST(const std::vector<int>& data,
                 const std::vector<int>& weights,
                 int left,
                 int right,
                 int level,
                 int* result) {
    if(left > right) return;

    int min_index = std::distance(data.begin(), std::min_element(data.begin() + left, data.begin() + right + 1));

    *result += (level * weights[min_index]);

    OptionalBST(data, weights, left, min_index - 1, level + 1, result);
    OptionalBST(data, weights, min_index + 1, right, level + 1, result);
}

int OptionalBST(const std::vector<int>& data,
                const std::vector<int>& weights) {
    int result {0};
    OptionalBST(data, weights, 0, data.size() - 1, 1, &result);
    return result;
}

TEST(OptionalBSTTest, TrivialCases) {
    EXPECT_EQ(OptionalBST({}, {}), 0);
    EXPECT_EQ(OptionalBST({10}, {20}), 10 * 20);
}


int OptionalBSTDP(const std::vector<int>& data,
                  const std::vector<int>& weights,
                  int left, int right,
                  int level,
                  std::vector<std::vector<int>>* table) {
    if(left > right) return 0;

    if((*table)[left][right] != -1) {
        return (*table)[left][right];
    }

    int min_result = INT_MAX; // std::numeric_limits<int>::max(); // -> it's better

    for(int i = left; i <= right; ++i) {
        int cost = weights[i] + OptionalBSTDP(data, weights, left, i - 1, level + 1, table)
                        + OptionalBSTDP(data, weights, i + 1, right, level + 1, table);
        min_result = std::min(min_result, cost);
    }

    return min_result;
}

int OptionalBSTDP(const std::vector<int>& data,
                  const std::vector<int>& weights) {

    std::vector<std::vector<int>> table(data.size(), std::vector<int>(data.size()));

    for(std::size_t i = 0; i < table.size(); ++i) {
        std::find(table[i].begin(), table[i].end(), -1);
    }

    return OptionalBSTDP(data, weights, 0, data.size() - 1, 1, &table);

}

int main() {


    std::vector<int> data {1, 2, 3, 4, 0, 5, 6, 7, 8};

    int index = std::distance(data.begin(), std::min_element(data.begin(), data.end()));
    std::cout << data[index] << std::endl;

    BST<int> one;
    one.Insert(10);
    one.Insert(5);
    one.Insert(15);

    BST<int> two;
    two.Insert(10);
    two.Insert(55);
    two.Insert(15);


    if(one.IsSame(two)) {
        std::cout << "They're same" << std::endl;
    } else {
        std::cout << "They're not same" << std::endl;
    }

    return 0;
}