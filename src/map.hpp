#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key> >
class map {
public:
    typedef pair<const Key, T> value_type;
private:

    struct Node {
        value_type* data;
        Node* left;
        Node* right;
        Node* parent;
        int height;
        Node() : data(nullptr), left(nullptr), right(nullptr), parent(nullptr), height(1) {}
        Node(const value_type& v) : data(new value_type(v)), left(nullptr), right(nullptr), parent(nullptr), height(1) {}
        ~Node() { if (data) delete data; }
    };

    Node* root;
    Node* header;
    size_t sz;

    static int height(Node* n) { return n ? n->height : 0; }
    static void updateHeight(Node* n) {
        int l = height(n->left), r = height(n->right);
        n->height = 1 + (l > r ? l : r);
    }
    static int balanceFactor(Node* n) { return height(n->left) - height(n->right); }

    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        if (x->right) x->right->parent = y;
        x->right = y;
        x->parent = y->parent;
        y->parent = x;
        updateHeight(y);
        updateHeight(x);
        return x;
    }
    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;
        y->left = x;
        y->parent = x->parent;
        x->parent = y;
        updateHeight(x);
        updateHeight(y);
        return y;
    }
    static Node* balance(Node* n) {
        updateHeight(n);
        int bf = balanceFactor(n);
        if (bf > 1) {
            if (balanceFactor(n->left) < 0) n->left = rotateLeft(n->left);
            return rotateRight(n);
        } else if (bf < -1) {
            if (balanceFactor(n->right) > 0) n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;
    }

    Node* insertRec(Node* node, const value_type& v, Node*& newNode, bool& inserted) {
        if (!node) {
            node = new Node(v);
            newNode = node;
            inserted = true;
            return node;
        }
        if (Compare()(v.first, node->data->first)) {
            node->left = insertRec(node->left, v, newNode, inserted);
            if (node->left) node->left->parent = node;
        } else if (Compare()(node->data->first, v.first)) {
            node->right = insertRec(node->right, v, newNode, inserted);
            if (node->right) node->right->parent = node;
        } else {
            newNode = node;
            inserted = false;
            return node;
        }
        return balance(node);
    }

    void rebalanceFrom(Node* node) {
        while (node && node != header) {
            Node* p = node->parent;
            bool leftChild = (p && p->left == node);
            node = balance(node);
            if (p == nullptr) root = node;
            else if (p == header) root = node;
            else if (leftChild) p->left = node;
            else p->right = node;
            node = p;
        }
    }

    static Node* minNode(Node* n) { while (n && n->left) n = n->left; return n; }
    static Node* maxNode(Node* n) { while (n && n->right) n = n->right; return n; }

    void updateHeader() {
        header->parent = root;
        if (root) {
            root->parent = header;
            header->left = minNode(root);
            header->right = maxNode(root);
        } else {
            header->left = header;
            header->right = header;
        }
    }

    Node* copyTree(Node* src, Node* parent) {
        if (!src) return nullptr;
        Node* n = new Node(*src->data);
        n->parent = parent;
        n->height = src->height;
        n->left = copyTree(src->left, n);
        n->right = copyTree(src->right, n);
        return n;
    }

    void clearRec(Node* n) {
        if (!n) return;
        clearRec(n->left);
        clearRec(n->right);
        delete n;
    }

    Node* findNode(const Key& key) const {
        Node* cur = root;
        while (cur) {
            if (Compare()(key, cur->data->first)) cur = cur->left;
            else if (Compare()(cur->data->first, key)) cur = cur->right;
            else return cur;
        }
        return nullptr;
    }

    void eraseNode(Node* z) {
        if (!z->left || !z->right) {
            Node* x = z->left ? z->left : z->right;
            Node* p = z->parent;
            bool isRoot = (p == header);
            if (x) x->parent = isRoot ? header : p;
            if (isRoot) root = x;
            else if (p->left == z) p->left = x;
            else p->right = x;
            delete z;
            if (isRoot) rebalanceFrom(header);
            else rebalanceFrom(p);
        } else {
            Node* y = z->right;
            while (y->left) y = y->left;
            if (y->parent == z) {
                Node* x = y->right;
                Node* gp = z->parent;
                bool isRoot = (gp == header);
                if (x) x->parent = y;
                y->left = z->left;
                if (z->left) z->left->parent = y;
                y->parent = isRoot ? header : gp;
                if (isRoot) root = y;
                else if (gp->left == z) gp->left = y;
                else gp->right = y;
                delete z;
                if (isRoot) rebalanceFrom(header);
                else rebalanceFrom(gp);
            } else {
                Node* yp = y->parent;
                Node* x = y->right;
                if (x) x->parent = yp;
                if (yp->left == y) yp->left = x;
                else yp->right = x;
                y->left = z->left;
                if (z->left) z->left->parent = y;
                y->right = z->right;
                if (z->right) z->right->parent = y;
                y->parent = z->parent;
                if (z->parent == header) root = y;
                else if (z->parent->left == z) z->parent->left = y;
                else z->parent->right = y;
                delete z;
                rebalanceFrom(yp);
            }
        }
        sz--;
        updateHeader();
    }

public:
    class const_iterator;
    class iterator {
    public:
        Node* ptr;
        Node* header;
        iterator() : ptr(nullptr), header(nullptr) {}
        iterator(Node* p, Node* h) : ptr(p), header(h) {}
        iterator(const iterator &other) : ptr(other.ptr), header(other.header) {}

        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
        iterator &operator++() {
            if (ptr == nullptr) throw invalid_iterator();
            if (ptr == header) throw invalid_iterator();
            if (ptr->right) {
                ptr = ptr->right;
                while (ptr->left) ptr = ptr->left;
            } else {
                Node* y = ptr->parent;
                while (y && y->right == ptr) { ptr = y; y = y->parent; }
                ptr = y;
            }
            return *this;
        }
        iterator operator--(int) { iterator tmp = *this; --(*this); return tmp; }
        iterator &operator--() {
            if (ptr == nullptr) throw invalid_iterator();
            if (ptr == header) {
                if (header->right == header) throw invalid_iterator();
                ptr = header->right;
                return *this;
            }
            if (ptr == header->left) throw invalid_iterator();
            if (ptr->left) {
                ptr = ptr->left;
                while (ptr->right) ptr = ptr->right;
            } else {
                Node* y = ptr->parent;
                while (y && y->left == ptr) { ptr = y; y = y->parent; }
                ptr = y;
            }
            return *this;
        }
        value_type &operator*() const {
            if (ptr == nullptr || ptr == header) throw invalid_iterator();
            return *(ptr->data);
        }
        value_type *operator->() const {
            if (ptr == nullptr || ptr == header) throw invalid_iterator();
            return ptr->data;
        }
        bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
        bool operator==(const const_iterator &rhs) const;
        bool operator!=(const iterator &rhs) const { return ptr != rhs.ptr; }
        bool operator!=(const const_iterator &rhs) const;
    };

    class const_iterator {
    public:
        Node* ptr;
        Node* header;
        const_iterator() : ptr(nullptr), header(nullptr) {}
        const_iterator(Node* p, Node* h) : ptr(p), header(h) {}
        const_iterator(const const_iterator &other) : ptr(other.ptr), header(other.header) {}
        const_iterator(const iterator &other) : ptr(other.ptr), header(other.header) {}

        const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }
        const_iterator &operator++() {
            if (ptr == nullptr) throw invalid_iterator();
            if (ptr == header) throw invalid_iterator();
            if (ptr->right) {
                ptr = ptr->right;
                while (ptr->left) ptr = ptr->left;
            } else {
                Node* y = ptr->parent;
                while (y && y->right == ptr) { ptr = y; y = y->parent; }
                ptr = y;
            }
            return *this;
        }
        const_iterator operator--(int) { const_iterator tmp = *this; --(*this); return tmp; }
        const_iterator &operator--() {
            if (ptr == nullptr) throw invalid_iterator();
            if (ptr == header) {
                if (header->right == header) throw invalid_iterator();
                ptr = header->right;
                return *this;
            }
            if (ptr == header->left) throw invalid_iterator();
            if (ptr->left) {
                ptr = ptr->left;
                while (ptr->right) ptr = ptr->right;
            } else {
                Node* y = ptr->parent;
                while (y && y->left == ptr) { ptr = y; y = y->parent; }
                ptr = y;
            }
            return *this;
        }
        const value_type &operator*() const {
            if (ptr == nullptr || ptr == header) throw invalid_iterator();
            return *(ptr->data);
        }
        const value_type *operator->() const {
            if (ptr == nullptr || ptr == header) throw invalid_iterator();
            return ptr->data;
        }
        bool operator==(const const_iterator &rhs) const { return ptr == rhs.ptr; }
        bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
        bool operator!=(const const_iterator &rhs) const { return ptr != rhs.ptr; }
        bool operator!=(const iterator &rhs) const { return ptr != rhs.ptr; }
    };

    map() {
        sz = 0;
        root = nullptr;
        header = new Node();
        header->left = header;
        header->right = header;
        header->parent = nullptr;
    }

    map(const map &other) {
        sz = other.sz;
        header = new Node();
        root = copyTree(other.root, nullptr);
        updateHeader();
    }

    map &operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        sz = other.sz;
        root = copyTree(other.root, nullptr);
        updateHeader();
        return *this;
    }

    ~map() {
        clear();
        delete header;
    }

    T &at(const Key &key) {
        Node* n = findNode(key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }
    const T &at(const Key &key) const {
        Node* n = findNode(key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }

    T &operator[](const Key &key) {
        Node* n = findNode(key);
        if (n) return n->data->second;
        value_type v(key, T());
        pair<iterator, bool> res = insert(v);
        return res.first.ptr->data->second;
    }
    const T &operator[](const Key &key) const {
        Node* n = findNode(key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }

    iterator begin() { return iterator(header->left, header); }
    const_iterator cbegin() const { return const_iterator(header->left, header); }
    const_iterator begin() const { return const_iterator(header->left, header); }

    iterator end() { return iterator(header, header); }
    const_iterator cend() const { return const_iterator(header, header); }
    const_iterator end() const { return const_iterator(header, header); }

    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }

    void clear() {
        clearRec(root);
        root = nullptr;
        sz = 0;
        header->parent = nullptr;
        header->left = header;
        header->right = header;
    }

    pair<iterator, bool> insert(const value_type &value) {
        Node* newNode = nullptr;
        bool inserted = false;
        root = insertRec(root, value, newNode, inserted);
        updateHeader();
        if (inserted) sz++;
        return pair<iterator, bool>(iterator(newNode, header), inserted);
    }

    void erase(iterator pos) {
        if (pos.ptr == nullptr || pos.header != header) throw invalid_iterator();
        if (pos.ptr == header) throw invalid_iterator();
        eraseNode(pos.ptr);
    }

    size_t count(const Key &key) const {
        return findNode(key) ? 1 : 0;
    }

    iterator find(const Key &key) {
        Node* n = findNode(key);
        if (!n) return end();
        return iterator(n, header);
    }
    const_iterator find(const Key &key) const {
        Node* n = findNode(key);
        if (!n) return cend();
        return const_iterator(n, header);
    }
};

template<class Key, class T, class Compare>
bool map<Key,T,Compare>::iterator::operator==(const typename map<Key,T,Compare>::const_iterator &rhs) const {
    return ptr == rhs.ptr;
}

template<class Key, class T, class Compare>
bool map<Key,T,Compare>::iterator::operator!=(const typename map<Key,T,Compare>::const_iterator &rhs) const {
    return ptr != rhs.ptr;
}

}

#endif
