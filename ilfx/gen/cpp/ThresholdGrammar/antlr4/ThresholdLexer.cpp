
// Generated from antlr4/Threshold.g4 by ANTLR 4.13.2


#include "ThresholdLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct ThresholdLexerStaticData final {
  ThresholdLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  ThresholdLexerStaticData(const ThresholdLexerStaticData&) = delete;
  ThresholdLexerStaticData(ThresholdLexerStaticData&&) = delete;
  ThresholdLexerStaticData& operator=(const ThresholdLexerStaticData&) = delete;
  ThresholdLexerStaticData& operator=(ThresholdLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag thresholdlexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<ThresholdLexerStaticData> thresholdlexerLexerStaticData = nullptr;

void thresholdlexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (thresholdlexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(thresholdlexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<ThresholdLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "T__6", "T__7", "AND", 
      "OR", "FLOAT", "INT", "IDENT", "WS"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "':'", "'('", "')'", "'<'", "'<='", "'>'", "'>='", "'=='"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "AND", "OR", "FLOAT", "INT", "IDENT", 
      "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,14,91,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,1,0,
  	1,0,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,4,1,5,1,5,1,6,1,6,1,6,1,7,1,7,1,
  	7,1,8,1,8,1,8,1,8,1,8,3,8,54,8,8,1,9,1,9,1,9,1,9,3,9,60,8,9,1,10,4,10,
  	63,8,10,11,10,12,10,64,1,10,1,10,4,10,69,8,10,11,10,12,10,70,1,11,4,11,
  	74,8,11,11,11,12,11,75,1,12,1,12,5,12,80,8,12,10,12,12,12,83,9,12,1,13,
  	4,13,86,8,13,11,13,12,13,87,1,13,1,13,0,0,14,1,1,3,2,5,3,7,4,9,5,11,6,
  	13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,14,1,0,9,2,0,65,65,97,97,2,
  	0,78,78,110,110,2,0,68,68,100,100,2,0,79,79,111,111,2,0,82,82,114,114,
  	1,0,48,57,3,0,65,90,95,95,97,122,4,0,48,57,65,90,95,95,97,122,3,0,9,10,
  	13,13,32,32,97,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,
  	0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,1,0,0,
  	0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,0,0,27,1,0,0,0,1,29,1,0,0,0,3,
  	31,1,0,0,0,5,33,1,0,0,0,7,35,1,0,0,0,9,37,1,0,0,0,11,40,1,0,0,0,13,42,
  	1,0,0,0,15,45,1,0,0,0,17,53,1,0,0,0,19,59,1,0,0,0,21,62,1,0,0,0,23,73,
  	1,0,0,0,25,77,1,0,0,0,27,85,1,0,0,0,29,30,5,58,0,0,30,2,1,0,0,0,31,32,
  	5,40,0,0,32,4,1,0,0,0,33,34,5,41,0,0,34,6,1,0,0,0,35,36,5,60,0,0,36,8,
  	1,0,0,0,37,38,5,60,0,0,38,39,5,61,0,0,39,10,1,0,0,0,40,41,5,62,0,0,41,
  	12,1,0,0,0,42,43,5,62,0,0,43,44,5,61,0,0,44,14,1,0,0,0,45,46,5,61,0,0,
  	46,47,5,61,0,0,47,16,1,0,0,0,48,49,7,0,0,0,49,50,7,1,0,0,50,54,7,2,0,
  	0,51,52,5,38,0,0,52,54,5,38,0,0,53,48,1,0,0,0,53,51,1,0,0,0,54,18,1,0,
  	0,0,55,56,7,3,0,0,56,60,7,4,0,0,57,58,5,124,0,0,58,60,5,124,0,0,59,55,
  	1,0,0,0,59,57,1,0,0,0,60,20,1,0,0,0,61,63,7,5,0,0,62,61,1,0,0,0,63,64,
  	1,0,0,0,64,62,1,0,0,0,64,65,1,0,0,0,65,66,1,0,0,0,66,68,5,46,0,0,67,69,
  	7,5,0,0,68,67,1,0,0,0,69,70,1,0,0,0,70,68,1,0,0,0,70,71,1,0,0,0,71,22,
  	1,0,0,0,72,74,7,5,0,0,73,72,1,0,0,0,74,75,1,0,0,0,75,73,1,0,0,0,75,76,
  	1,0,0,0,76,24,1,0,0,0,77,81,7,6,0,0,78,80,7,7,0,0,79,78,1,0,0,0,80,83,
  	1,0,0,0,81,79,1,0,0,0,81,82,1,0,0,0,82,26,1,0,0,0,83,81,1,0,0,0,84,86,
  	7,8,0,0,85,84,1,0,0,0,86,87,1,0,0,0,87,85,1,0,0,0,87,88,1,0,0,0,88,89,
  	1,0,0,0,89,90,6,13,0,0,90,28,1,0,0,0,8,0,53,59,64,70,75,81,87,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  thresholdlexerLexerStaticData = std::move(staticData);
}

}

ThresholdLexer::ThresholdLexer(CharStream *input) : Lexer(input) {
  ThresholdLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *thresholdlexerLexerStaticData->atn, thresholdlexerLexerStaticData->decisionToDFA, thresholdlexerLexerStaticData->sharedContextCache);
}

ThresholdLexer::~ThresholdLexer() {
  delete _interpreter;
}

std::string ThresholdLexer::getGrammarFileName() const {
  return "Threshold.g4";
}

const std::vector<std::string>& ThresholdLexer::getRuleNames() const {
  return thresholdlexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& ThresholdLexer::getChannelNames() const {
  return thresholdlexerLexerStaticData->channelNames;
}

const std::vector<std::string>& ThresholdLexer::getModeNames() const {
  return thresholdlexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& ThresholdLexer::getVocabulary() const {
  return thresholdlexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView ThresholdLexer::getSerializedATN() const {
  return thresholdlexerLexerStaticData->serializedATN;
}

const atn::ATN& ThresholdLexer::getATN() const {
  return *thresholdlexerLexerStaticData->atn;
}




void ThresholdLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  thresholdlexerLexerInitialize();
#else
  ::antlr4::internal::call_once(thresholdlexerLexerOnceFlag, thresholdlexerLexerInitialize);
#endif
}
