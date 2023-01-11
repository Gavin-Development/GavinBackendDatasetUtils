#include "DataLoader.hpp"

/* *** Helper functions. *** */

inline int64_t Tokenizer::_PieceInVocab(std::string Piece) {
	for (size_t i = 0; i < _Vocab.size(); i++) {
		if (Piece == _Vocab[i].second) return i;
	}
	return -1;
}

inline int64_t Tokenizer::_WordInCorpus(std::vector<std::string> Word, std::vector<std::pair<uint64_t, std::vector<std::string>>> Corpus) {
	bool Match;

	for (size_t i = 0; i < Corpus.size(); i++) {

		Match = true;
		if (Corpus[i].second.size() != Word.size()) { Match = false; continue; }

		for (size_t j = 0; j < Corpus[i].second.size(); j++) {
			if (Corpus[i].second[j] != Word[j]) { Match = false; continue; }
		}

		if (Match == true) return i;
	}

	return -1;
}

inline int64_t Tokenizer::_PairInPairs(std::pair<std::string, std::string> Pair, std::vector<std::pair<uint64_t, std::pair<std::string, std::string>>> Pairs) {
	for (size_t i = 0; i < Pairs.size(); i++) {
		if (Pair == Pairs[i].second) return i;
	}
	return -1;
}


/* *** Main Functions *** */

void Tokenizer::Build(std::vector<std::string>& Words) {
#ifdef _DEBUG
	std::cout << "Called Tokenizer.Build()." << std::endl;
#endif // _DEBUG

	std::vector<std::pair<uint64_t, std::vector<std::string>>> Corpus;
	
	// Create the vocab.
	std::vector<std::string> SplitWord;
	int64_t PiecePosition, SplitWordPos;
	for (std::string Word : Words) {
		SplitWord.erase(SplitWord.begin(), SplitWord.end());

		// Push back the first letter of the word.
		SplitWord.push_back(Word.substr(0, 1));

		// Form the base vocabulary.
		PiecePosition = _PieceInVocab(Word.substr(0, 1));
		if (PiecePosition == -1) _Vocab.push_back({ 1, Word.substr(0,1) });
		else _Vocab[PiecePosition].first++;

		for (size_t i = 1; i < Word.length(); i++) {
			PiecePosition = _PieceInVocab("##" + Word.substr(i, 1));
			if (PiecePosition == -1) _Vocab.push_back({ 1, "##" + Word.substr(i,1) });
			else _Vocab[PiecePosition].first++;

			SplitWord.push_back("##" + Word.substr(i, 1));
		}

		// Form the corpus.
		SplitWordPos = _WordInCorpus(SplitWord, Corpus);
		if (SplitWordPos == -1) Corpus.push_back({ 1, SplitWord });
		else Corpus[SplitWordPos].first++;
	}

	// Extend the vocab.
	std::vector<std::pair<uint64_t, std::pair<std::string, std::string>>> Pairs;
	std::vector<std::string> MergedWord;
	int64_t PairPos;
	double Score, HighScore;
	size_t HighScoreIndex;
	bool WordHas2Pieces;

	for (size_t i = 0; i < _TargetVocabSize; i++) {
		// Reset used variables.
		Pairs.erase(Pairs.begin(), Pairs.end());
		Score = 0, HighScore = 0;
		WordHas2Pieces = false;

		// Find the commonality of each pair of Pieces.
		for (auto Word : Corpus) {
			if (Word.second.size() != 1) { WordHas2Pieces = true; }
			else continue;
			for (size_t j = 0; j < Word.second.size() - 1; j++) {
				PairPos = _PairInPairs({ Word.second[j], Word.second[j + 1] }, Pairs);

				if (PairPos == -1) Pairs.push_back({ Word.first, {Word.second[j], Word.second[j + 1]} });
				else Pairs[PairPos].first += Word.first;
			}
		}
		// If there are no more possible merges then we break the loop.
		if (WordHas2Pieces == false) break;

		// Compute the scores of each potential pair & determine the pair with the highest score.
		for (size_t j = 0; j < Pairs.size(); j++) {
			Score = (double)Pairs[j].first / (_Vocab[_PieceInVocab(Pairs[j].second.first)].first * _Vocab[_PieceInVocab(Pairs[j].second.second)].first);

			if (Score > HighScore) { HighScore = Score; HighScoreIndex = j; }
		}

		// Merge the highest scoring pair.
		for (auto& Word : Corpus) {
			MergedWord.clear();
			for (size_t j = 0; j < Word.second.size(); j++) {
				if (j < Word.second.size() - 1) {
					if (Word.second[j] == Pairs[HighScoreIndex].second.first && Word.second[j + 1] == Pairs[HighScoreIndex].second.second) {
						MergedWord.push_back(Word.second[j] + Word.second[j + 1].substr(2, Word.second[j + 1].length()));
						j++;
					}
					else MergedWord.push_back(Word.second[j]);
				}
				else MergedWord.push_back(Word.second[j]);
			}
			Word.second = MergedWord;
		}

		// Append the new Piece to the Vocab.
		_Vocab.push_back({ Pairs[HighScoreIndex].first, Pairs[HighScoreIndex].second.first + Pairs[HighScoreIndex].second.second.substr(2,Pairs[HighScoreIndex].second.second.length()) });
	}

	for (auto Piece : _Vocab) {
		std::cout << "Piece: " << Piece.second << "  Occurances: " << Piece.first << std::endl;
	}
}

std::vector<uint64_t> Tokenizer::Encode(std::vector<std::string> Words) {
	std::vector<uint64_t> Encodes;

	int64_t EncodePos;
	bool EncodeComplete;
	size_t TestStartPos;

	for (size_t i = 0; i < Words.size(); i++) {

		EncodeComplete = false;
		TestStartPos = 0;

		// Check the first bit of the word.
		for (size_t j = Words[i].size(); j > 0; j--) {
			EncodePos = _PieceInVocab(Words[i].substr(0, j));

			if (EncodePos != -1) { Encodes.push_back(EncodePos); TestStartPos = j; break; }
		}

		// Check the next n sections of the word.
		while (!EncodeComplete) {
			for (size_t j = Words[i].size() - TestStartPos; j > 0; j--) {
				EncodePos = _PieceInVocab("##" + Words[i].substr(TestStartPos, j));

				std::cout << "##" + Words[i].substr(TestStartPos, j) << std::endl;

				if (EncodePos != -1) { Encodes.push_back(EncodePos); TestStartPos += j; break; }
				if (j == 1) { Encodes.push_back(UnknownToken.first); TestStartPos += j; break; }
			}
			if (TestStartPos > Words[i].size() - 1) EncodeComplete = true;
		}
	}

	return Encodes;
};

std::vector<std::string> Tokenizer::Decode(py::array_t<uint64_t> Encodes) {
	std::vector<std::string> Decodes;

	for (size_t i = 0; i < Encodes.size(); i++) {

		// If its an UnknownToken token.
		if (Encodes.at(i) == UnknownToken.first) { Decodes.push_back(UnknownToken.second); continue; }

		// If its not an unknown token.

		// If it is a new word.
		// Check that the proposed Decode is of adequate length for testing.
		if (_Vocab[Encodes.at(i)].second.length() >= 3) {
			if (_Vocab[Encodes.at(i)].second.at(0) != (char&)"#" && _Vocab[Encodes.at(i)].second.at(1) != (char&)"#") {
				// Push back the new word start.
				Decodes.push_back(_Vocab[Encodes.at(i)].second);
			}
			else { Decodes[Decodes.size() - 1] += _Vocab[Encodes.at(i)].second.substr(2, _Vocab[Encodes.at(i)].second.length() - 2); }
		}
		// If the token is not a new word start.
		else {
			Decodes[Decodes.size() - 1] += _Vocab[Encodes.at(i)].second.substr(2,_Vocab[Encodes.at(i)].second.length()-2);
		}
	}

	return Decodes;
}