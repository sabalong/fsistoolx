
// Generated from antlr4/Threshold.g4 by ANTLR 4.13.2


#include "ThresholdVisitor.h"

#include "ThresholdParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct ThresholdParserStaticData final {
  ThresholdParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  ThresholdParserStaticData(const ThresholdParserStaticData&) = delete;
  ThresholdParserStaticData(ThresholdParserStaticData&&) = delete;
  ThresholdParserStaticData& operator=(const ThresholdParserStaticData&) = delete;
  ThresholdParserStaticData& operator=(ThresholdParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag thresholdParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<ThresholdParserStaticData> thresholdParserStaticData = nullptr;

void thresholdParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (thresholdParserStaticData != nullptr) {
    return;
  }
#else
  assert(thresholdParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<ThresholdParserStaticData>(
    std::vector<std::string>{
      "ruleFile", "ruleDecl", "rating", "expr", "orExpr", "andExpr", "comparison", 
      "value", "op"
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
  	4,1,14,67,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,7,
  	7,7,2,8,7,8,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,4,5,
  	4,33,8,4,10,4,12,4,36,9,4,1,5,1,5,1,5,5,5,41,8,5,10,5,12,5,44,9,5,1,6,
  	1,6,1,6,1,6,1,6,1,6,5,6,52,8,6,10,6,12,6,55,9,6,1,6,1,6,1,6,1,6,3,6,61,
  	8,6,1,7,1,7,1,8,1,8,1,8,0,0,9,0,2,4,6,8,10,12,14,16,0,2,1,0,11,13,1,0,
  	4,8,61,0,18,1,0,0,0,2,21,1,0,0,0,4,25,1,0,0,0,6,27,1,0,0,0,8,29,1,0,0,
  	0,10,37,1,0,0,0,12,60,1,0,0,0,14,62,1,0,0,0,16,64,1,0,0,0,18,19,3,2,1,
  	0,19,20,5,0,0,1,20,1,1,0,0,0,21,22,3,4,2,0,22,23,5,1,0,0,23,24,3,6,3,
  	0,24,3,1,0,0,0,25,26,5,12,0,0,26,5,1,0,0,0,27,28,3,8,4,0,28,7,1,0,0,0,
  	29,34,3,10,5,0,30,31,5,10,0,0,31,33,3,10,5,0,32,30,1,0,0,0,33,36,1,0,
  	0,0,34,32,1,0,0,0,34,35,1,0,0,0,35,9,1,0,0,0,36,34,1,0,0,0,37,42,3,12,
  	6,0,38,39,5,9,0,0,39,41,3,12,6,0,40,38,1,0,0,0,41,44,1,0,0,0,42,40,1,
  	0,0,0,42,43,1,0,0,0,43,11,1,0,0,0,44,42,1,0,0,0,45,46,3,14,7,0,46,47,
  	3,16,8,0,47,53,3,14,7,0,48,49,3,16,8,0,49,50,3,14,7,0,50,52,1,0,0,0,51,
  	48,1,0,0,0,52,55,1,0,0,0,53,51,1,0,0,0,53,54,1,0,0,0,54,61,1,0,0,0,55,
  	53,1,0,0,0,56,57,5,2,0,0,57,58,3,6,3,0,58,59,5,3,0,0,59,61,1,0,0,0,60,
  	45,1,0,0,0,60,56,1,0,0,0,61,13,1,0,0,0,62,63,7,0,0,0,63,15,1,0,0,0,64,
  	65,7,1,0,0,65,17,1,0,0,0,4,34,42,53,60
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  thresholdParserStaticData = std::move(staticData);
}

}

ThresholdParser::ThresholdParser(TokenStream *input) : ThresholdParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

ThresholdParser::ThresholdParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  ThresholdParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *thresholdParserStaticData->atn, thresholdParserStaticData->decisionToDFA, thresholdParserStaticData->sharedContextCache, options);
}

ThresholdParser::~ThresholdParser() {
  delete _interpreter;
}

const atn::ATN& ThresholdParser::getATN() const {
  return *thresholdParserStaticData->atn;
}

std::string ThresholdParser::getGrammarFileName() const {
  return "Threshold.g4";
}

const std::vector<std::string>& ThresholdParser::getRuleNames() const {
  return thresholdParserStaticData->ruleNames;
}

const dfa::Vocabulary& ThresholdParser::getVocabulary() const {
  return thresholdParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView ThresholdParser::getSerializedATN() const {
  return thresholdParserStaticData->serializedATN;
}


//----------------- RuleFileContext ------------------------------------------------------------------

ThresholdParser::RuleFileContext::RuleFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ThresholdParser::RuleDeclContext* ThresholdParser::RuleFileContext::ruleDecl() {
  return getRuleContext<ThresholdParser::RuleDeclContext>(0);
}

tree::TerminalNode* ThresholdParser::RuleFileContext::EOF() {
  return getToken(ThresholdParser::EOF, 0);
}


size_t ThresholdParser::RuleFileContext::getRuleIndex() const {
  return ThresholdParser::RuleRuleFile;
}


std::any ThresholdParser::RuleFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitRuleFile(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::RuleFileContext* ThresholdParser::ruleFile() {
  RuleFileContext *_localctx = _tracker.createInstance<RuleFileContext>(_ctx, getState());
  enterRule(_localctx, 0, ThresholdParser::RuleRuleFile);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(18);
    ruleDecl();
    setState(19);
    match(ThresholdParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RuleDeclContext ------------------------------------------------------------------

ThresholdParser::RuleDeclContext::RuleDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ThresholdParser::RatingContext* ThresholdParser::RuleDeclContext::rating() {
  return getRuleContext<ThresholdParser::RatingContext>(0);
}

ThresholdParser::ExprContext* ThresholdParser::RuleDeclContext::expr() {
  return getRuleContext<ThresholdParser::ExprContext>(0);
}


size_t ThresholdParser::RuleDeclContext::getRuleIndex() const {
  return ThresholdParser::RuleRuleDecl;
}


std::any ThresholdParser::RuleDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitRuleDecl(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::RuleDeclContext* ThresholdParser::ruleDecl() {
  RuleDeclContext *_localctx = _tracker.createInstance<RuleDeclContext>(_ctx, getState());
  enterRule(_localctx, 2, ThresholdParser::RuleRuleDecl);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(21);
    rating();
    setState(22);
    match(ThresholdParser::T__0);
    setState(23);
    expr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RatingContext ------------------------------------------------------------------

ThresholdParser::RatingContext::RatingContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ThresholdParser::RatingContext::INT() {
  return getToken(ThresholdParser::INT, 0);
}


size_t ThresholdParser::RatingContext::getRuleIndex() const {
  return ThresholdParser::RuleRating;
}


std::any ThresholdParser::RatingContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitRating(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::RatingContext* ThresholdParser::rating() {
  RatingContext *_localctx = _tracker.createInstance<RatingContext>(_ctx, getState());
  enterRule(_localctx, 4, ThresholdParser::RuleRating);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(25);
    match(ThresholdParser::INT);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExprContext ------------------------------------------------------------------

ThresholdParser::ExprContext::ExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ThresholdParser::OrExprContext* ThresholdParser::ExprContext::orExpr() {
  return getRuleContext<ThresholdParser::OrExprContext>(0);
}


size_t ThresholdParser::ExprContext::getRuleIndex() const {
  return ThresholdParser::RuleExpr;
}


std::any ThresholdParser::ExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitExpr(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::ExprContext* ThresholdParser::expr() {
  ExprContext *_localctx = _tracker.createInstance<ExprContext>(_ctx, getState());
  enterRule(_localctx, 6, ThresholdParser::RuleExpr);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(27);
    orExpr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OrExprContext ------------------------------------------------------------------

ThresholdParser::OrExprContext::OrExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<ThresholdParser::AndExprContext *> ThresholdParser::OrExprContext::andExpr() {
  return getRuleContexts<ThresholdParser::AndExprContext>();
}

ThresholdParser::AndExprContext* ThresholdParser::OrExprContext::andExpr(size_t i) {
  return getRuleContext<ThresholdParser::AndExprContext>(i);
}

std::vector<tree::TerminalNode *> ThresholdParser::OrExprContext::OR() {
  return getTokens(ThresholdParser::OR);
}

tree::TerminalNode* ThresholdParser::OrExprContext::OR(size_t i) {
  return getToken(ThresholdParser::OR, i);
}


size_t ThresholdParser::OrExprContext::getRuleIndex() const {
  return ThresholdParser::RuleOrExpr;
}


std::any ThresholdParser::OrExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitOrExpr(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::OrExprContext* ThresholdParser::orExpr() {
  OrExprContext *_localctx = _tracker.createInstance<OrExprContext>(_ctx, getState());
  enterRule(_localctx, 8, ThresholdParser::RuleOrExpr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(29);
    andExpr();
    setState(34);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ThresholdParser::OR) {
      setState(30);
      match(ThresholdParser::OR);
      setState(31);
      andExpr();
      setState(36);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AndExprContext ------------------------------------------------------------------

ThresholdParser::AndExprContext::AndExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<ThresholdParser::ComparisonContext *> ThresholdParser::AndExprContext::comparison() {
  return getRuleContexts<ThresholdParser::ComparisonContext>();
}

ThresholdParser::ComparisonContext* ThresholdParser::AndExprContext::comparison(size_t i) {
  return getRuleContext<ThresholdParser::ComparisonContext>(i);
}

std::vector<tree::TerminalNode *> ThresholdParser::AndExprContext::AND() {
  return getTokens(ThresholdParser::AND);
}

tree::TerminalNode* ThresholdParser::AndExprContext::AND(size_t i) {
  return getToken(ThresholdParser::AND, i);
}


size_t ThresholdParser::AndExprContext::getRuleIndex() const {
  return ThresholdParser::RuleAndExpr;
}


std::any ThresholdParser::AndExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitAndExpr(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::AndExprContext* ThresholdParser::andExpr() {
  AndExprContext *_localctx = _tracker.createInstance<AndExprContext>(_ctx, getState());
  enterRule(_localctx, 10, ThresholdParser::RuleAndExpr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(37);
    comparison();
    setState(42);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ThresholdParser::AND) {
      setState(38);
      match(ThresholdParser::AND);
      setState(39);
      comparison();
      setState(44);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ComparisonContext ------------------------------------------------------------------

ThresholdParser::ComparisonContext::ComparisonContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<ThresholdParser::ValueContext *> ThresholdParser::ComparisonContext::value() {
  return getRuleContexts<ThresholdParser::ValueContext>();
}

ThresholdParser::ValueContext* ThresholdParser::ComparisonContext::value(size_t i) {
  return getRuleContext<ThresholdParser::ValueContext>(i);
}

std::vector<ThresholdParser::OpContext *> ThresholdParser::ComparisonContext::op() {
  return getRuleContexts<ThresholdParser::OpContext>();
}

ThresholdParser::OpContext* ThresholdParser::ComparisonContext::op(size_t i) {
  return getRuleContext<ThresholdParser::OpContext>(i);
}

ThresholdParser::ExprContext* ThresholdParser::ComparisonContext::expr() {
  return getRuleContext<ThresholdParser::ExprContext>(0);
}


size_t ThresholdParser::ComparisonContext::getRuleIndex() const {
  return ThresholdParser::RuleComparison;
}


std::any ThresholdParser::ComparisonContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitComparison(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::ComparisonContext* ThresholdParser::comparison() {
  ComparisonContext *_localctx = _tracker.createInstance<ComparisonContext>(_ctx, getState());
  enterRule(_localctx, 12, ThresholdParser::RuleComparison);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(60);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case ThresholdParser::FLOAT:
      case ThresholdParser::INT:
      case ThresholdParser::IDENT: {
        enterOuterAlt(_localctx, 1);
        setState(45);
        value();
        setState(46);
        op();
        setState(47);
        value();
        setState(53);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 496) != 0)) {
          setState(48);
          op();
          setState(49);
          value();
          setState(55);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

      case ThresholdParser::T__1: {
        enterOuterAlt(_localctx, 2);
        setState(56);
        match(ThresholdParser::T__1);
        setState(57);
        expr();
        setState(58);
        match(ThresholdParser::T__2);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueContext ------------------------------------------------------------------

ThresholdParser::ValueContext::ValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ThresholdParser::ValueContext::INT() {
  return getToken(ThresholdParser::INT, 0);
}

tree::TerminalNode* ThresholdParser::ValueContext::FLOAT() {
  return getToken(ThresholdParser::FLOAT, 0);
}

tree::TerminalNode* ThresholdParser::ValueContext::IDENT() {
  return getToken(ThresholdParser::IDENT, 0);
}


size_t ThresholdParser::ValueContext::getRuleIndex() const {
  return ThresholdParser::RuleValue;
}


std::any ThresholdParser::ValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitValue(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::ValueContext* ThresholdParser::value() {
  ValueContext *_localctx = _tracker.createInstance<ValueContext>(_ctx, getState());
  enterRule(_localctx, 14, ThresholdParser::RuleValue);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(62);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 14336) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OpContext ------------------------------------------------------------------

ThresholdParser::OpContext::OpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t ThresholdParser::OpContext::getRuleIndex() const {
  return ThresholdParser::RuleOp;
}


std::any ThresholdParser::OpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<ThresholdVisitor*>(visitor))
    return parserVisitor->visitOp(this);
  else
    return visitor->visitChildren(this);
}

ThresholdParser::OpContext* ThresholdParser::op() {
  OpContext *_localctx = _tracker.createInstance<OpContext>(_ctx, getState());
  enterRule(_localctx, 16, ThresholdParser::RuleOp);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(64);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 496) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void ThresholdParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  thresholdParserInitialize();
#else
  ::antlr4::internal::call_once(thresholdParserOnceFlag, thresholdParserInitialize);
#endif
}
