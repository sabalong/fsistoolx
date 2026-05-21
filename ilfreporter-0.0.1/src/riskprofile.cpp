#include "riskprofile.hpp"

#include "chaiscript/chaiscript.hpp"
#include "chaiscript/extras/math.hpp"

#include <xercesc/dom/DOM.hpp>

#include <xqilla/xqilla-dom3.hpp>

#include <xsd/cxx/xml/string.hxx>
#include <fstream>

using namespace riskprofile;

InherentReportGenerator::InherentReportGenerator(/* args */) {}

InherentReportGenerator::~InherentReportGenerator() {}

void InherentReportGenerator::setupChaiScriptEvaluator(chaiscript::ChaiScript &chai)
{
  auto mathlib = chaiscript::extras::math::bootstrap();
  chai.add(mathlib);

  chai.add(chaiscript::fun(
               [this](const std::string &xpathExpr)
               {
                 return this->filterRiskProfileNodeXPath(xpathExpr);
               }),
           "retrieveNodesByXpath");
   
  chai.add(chaiscript::fun(
               [this](const std::vector<RiskProfileNodeType> nodes)
               {
                 return this->riskProfileNodeTypeAsChaiVar(nodes);
               }),
           "riskProfileNodeTypeAsChaiVar");
  
  chai.add(chaiscript::fun(
                [this](const std::string& threshold, double value)
                {
                  return this->ratingByThreshold(threshold, value);
                }),
           "ratingByThreshold");

  chai.add(chaiscript::fun(
                [this](const std::string& threshold, const std::map<std::string, chaiscript::Boxed_Value>& variables)
                {
                  return this->ratingByThresholdVars(threshold, variables);
                }),
           "ratingByThresholdVars");
  
}

std::vector<RiskProfileNodeType> riskprofile::InherentReportGenerator::filterRiskProfileNodeXPath(const std::string &xpathExpr)
{
  std::vector<RiskProfileNodeType> results;

  if (!riskProfileTree)
  {
    throw std::runtime_error("RiskProfileTree is not set");
  }

  xercesc::DOMElement *root(static_cast<xercesc::DOMElement *>(riskProfileTree->_node()));
  if (root == nullptr)
  {
    throw std::runtime_error("RiskProfileTree DOM root is null; parsing likely failed");
  }

  xercesc::DOMDocument *doc(root->getOwnerDocument());
  if (doc == nullptr)
  {
    throw std::runtime_error("RiskProfileTree DOM document is null");
  }

  xml_schema::dom::unique_ptr<XQillaNSResolver> resolver(
      (XQillaNSResolver *)doc->createNSResolver(root));

  // Set the namespace prefix for the people namespace that
  // we can use reliably in XPath expressions regardless of
  // what is used in XML documents.
  //
  resolver->addNamespaceBinding(
      xsd::cxx::xml::string("inherent").c_str(),
      xsd::cxx::xml::string("http://www.example.com/inherent").c_str());
  xml_schema::dom::unique_ptr<const XQillaExpression> expr(
      static_cast<const XQillaExpression *>(
          doc->createExpression(
              xsd::cxx::xml::string(xpathExpr).c_str(),
              resolver.get())));

  xml_schema::dom::unique_ptr<xercesc::DOMXPathResult> r(
      expr->evaluate(doc, xercesc::DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

  // Iterate over the result.
  //
  while (r->iterateNext())
  {
    const xercesc::DOMNode *n(r->getNodeValue());

    // Obtain the object model node corresponding to
    // this DOM node.
    //
    RiskProfileNodeType *p(
        static_cast<RiskProfileNodeType *>(
            n->getUserData(xml_schema::dom::tree_node_key)));

    // Print the data using the object model.
    //
    if (p != nullptr)
    {
      results.push_back(*p);
      std::cout << "Name: " << p->risiko_name().get() << std::endl;
    }
  }

  return results;
}

absl::Status InherentReportGenerator::generateReport()
{
  
  for (auto &it : this->riskProfileConf->aggregationConfig()){
    std::cout << "Aggregation Config Risiko Name: " << it.risikoName() <<std::endl;

    std::cout << " Score Rule: " << it.score_rule() << std::endl;

    chaiscript::ChaiScript chai;
    this->setupChaiScriptEvaluator(chai);

    auto result = chai.eval<double>(it.score_rule());

    it.computed_weighted_score(result);

    std::cout << "computed_weighted_score: " << it.computed_weighted_score().get() << std::endl;

    chaiscript::ChaiScript chai2;
    this->setupChaiScriptEvaluator(chai2);

    std::string threshold = it.rating_to_score();
    chai2.add(chaiscript::var(threshold), "threshold");

    auto rating = chai2.eval<int>(it.rating_rule());

    it.computed_rating(std::to_string(rating));

    std::cout << "computed_rating: " << it.computed_rating().get() << std::endl;

  }

  return absl::OkStatus();
}

absl::Status InherentReportGenerator::writeInherentRiskProfileConf(const std::string& outputPath)
{
  if (!riskProfileConf)
  {
    return absl::InvalidArgumentError("InherentRiskProfileConf is not set");
  }

  try
  {
    xml_schema::namespace_infomap map;
    map[""].name = "http://example.com/inherent";
    map[""].schema = "./xsd/InherentRiskProfileConf.xsd";

    // Serialize the data to XML file
    std::ofstream ofs(outputPath);
    if (!ofs.is_open())
    {
      std::cerr << "Error: Failed to open output file: " << outputPath << std::endl;
      return absl::InvalidArgumentError("Cannot open output file: " + outputPath);
    }

    inherent::conf::InherentRiskProfileConf_(ofs, *riskProfileConf, map);
    ofs.close();

    std::cout << "Results written to: " << outputPath << std::endl;
    return absl::OkStatus();
  }
  catch (const std::exception& e)
  {
    return absl::InternalError(std::string("Failed to write output file: ") + e.what());
  }
}

std::vector<chaiscript::Boxed_Value> riskprofile::InherentReportGenerator::riskProfileNodeTypeAsChaiVar(std::vector<RiskProfileNodeType> nodes)
{
  std::vector<chaiscript::Boxed_Value> boxedValues;

  for (auto &n : nodes){
    std::cout << "Converting Node: " << n.risiko_name().get() << std::endl;
    chaiscript::Boxed_Value bv = chaiscript::Boxed_Value(n);

    std::map<std::string, chaiscript::Boxed_Value> nodeMap;
    std::string profileID = n.Profile_ID();
    
    nodeMap["Profile_ID"] = chaiscript::var(profileID);
    nodeMap["Computed_Weighted_Score"] = chaiscript::var(0.0);
    if (n.computed_weighted_score().present()){
      std::cout << "  Has Computed_Weighted_Score: " << n.computed_weighted_score().get() << std::endl;

      std::string s (n.computed_weighted_score().get().begin(), n.computed_weighted_score().get().end());
      double d = std::stod(s);
      nodeMap["Computed_Weighted_Score"] = chaiscript::var(d);
     // nodeMap["Computed_Weighted_Score"] = chaiscript::var(1.0);
    }

    boxedValues.push_back(chaiscript::var(nodeMap));
    
  }

    return boxedValues;
}
