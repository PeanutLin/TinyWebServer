#include <gtest/gtest.h>
#include "../trieRoute.h"

TEST(TrieTest, TrieTest1) {
  Trie trie;
  std::string path = "/a/b/c";
  trie.insert(path);
  std::map<std::string, std::string> mp;
  bool isOk = trie.search(path, mp);
  EXPECT_EQ(true, isOk);
  EXPECT_EQ(0, mp.size());
}

TEST(TrieTest, TrieTest2) {
  Trie trie;
  std::string path = "/a/:b/c";
  trie.insert(path);
  std::map<std::string, std::string> mp;
  std::string rPath = "/a/bagabaga/c";
  bool isOk = trie.search(rPath, mp);
  EXPECT_EQ(true, isOk);
  EXPECT_EQ(1, mp.size());
  EXPECT_EQ("bagabaga", mp["b"]);
}

TEST(TrieTest, TrieTest3) {
  Trie trie;
  std::string path = "/a/:b/c";
  trie.insert(path);
  std::map<std::string, std::string> mp;
  std::string rPath = "/t/bagabaga/c";
  bool isOk = trie.search(rPath, mp);
  EXPECT_EQ(false, isOk);
}
int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}