#include <gtest/gtest.h>
#include <fstream>
#include <DataLoader.hpp>

std::list<std::string> load_text(std::string filename) {
    std::list<std::string> texts;
    std::ifstream input(filename);
    std::string lineBuffer;
    while (std::getline(input, lineBuffer)) {
        texts.push_back(lineBuffer);
    }
    return texts;
};

TEST(DataLoaders, multi_threaded_loading) {
    auto data = LoadTrainDataMT(800, "./",
                                 "Test.BIN", 69108,
                                 66109, 52, 0);
    EXPECT_EQ(data.shape()[1], 52);

    std::list<int> start_tokens;
    for (int i = 0; i < data.shape()[0]; ++i) {
        start_tokens.push_back(data.at(i, 0));
    }
    EXPECT_TRUE(std::all_of(start_tokens.begin(), start_tokens.end(),
                            [](int i) { return i == 69108; }));

    auto data2 = LoadTrainDataMT(800, "./",
                                 "Test.BIN", 69108,
                                 66109, 52, 0);
    EXPECT_EQ(data2.shape()[0], data.shape()[0]);
    EXPECT_EQ(data2.shape()[1], data.shape()[1]);
}


TEST(DataLoaders, single_threaded_loading) {
    auto data = LoadTrainDataST(800, "./",
                                "Test.BIN", 69108,
                                66109, 52, 0);

    EXPECT_EQ(data.shape()[1], 52);

    std::list<int> start_tokens;
    for (int i = 0; i < data.shape()[0]; ++i) {
        start_tokens.push_back(data.at(i, 0));
    }
    EXPECT_TRUE(std::all_of(start_tokens.begin(), start_tokens.end(),
                            [](int i) { return i == 69108; }));

    auto data2 = LoadTrainDataST(800, "./",
                                 "Test.BIN", 69108,
                                 66109, 52, 0);
    EXPECT_EQ(data2.shape()[0], data.shape()[0]);
    EXPECT_EQ(data2.shape()[1], data.shape()[1]);
}

TEST(DataLoaders, legacy_loading) {
    auto data = LoadTrainDataST_Legacy(800, "./",
                                "small.to", 69108,
                                66109, 52, 0);
    std::list<int> lengths;
    for (const auto& row: data) {
        lengths.push_back(row.size());
    }
    EXPECT_TRUE(std::all_of(lengths.begin(), lengths.end(),
                            [](int i) { return i == 52; }));

    auto data2 = LoadTrainDataST_Legacy(800, "./",
                                 "small.to", 69108,
                                 66109, 52, 0);
    EXPECT_EQ(data2.size(), data.size());
    std::list<int> lengths2;
    for (const auto& row: data2) {
        lengths2.push_back(row.size());
    }
    EXPECT_TRUE(std::all_of(lengths2.begin(), lengths2.end(),
                            [](int i) { return i == 52; }));
}


int main(int argc, char **argv) {
    py::scoped_interpreter guard{};
    Py_Initialize();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
