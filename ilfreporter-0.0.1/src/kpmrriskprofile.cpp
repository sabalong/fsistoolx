#include "riskprofile.hpp"

#include "chaiscript/chaiscript.hpp"
#include "chaiscript/extras/math.hpp"

#include <xercesc/dom/DOM.hpp>
#include <xqilla/xqilla-dom3.hpp>
#include <xsd/cxx/xml/string.hxx>
#include <fstream>

riskprofile::KPMRReportGenerator::KPMRReportGenerator(){}

riskprofile::KPMRReportGenerator::~KPMRReportGenerator() {}

void riskprofile::KPMRReportGenerator::setupChaiScriptEvaluator(chaiscript::ChaiScript &chai)
{
    auto mathlib = chaiscript::extras::math::bootstrap();
    chai.add(mathlib);

    chai.add(chaiscript::fun(
                [this](const std::string &xpathExpr)
                {
                    return this->filterKPMRRiskProfileNodeXPath(xpathExpr);
                }),
            "retrieveNodesByXpath");
   
    chai.add(chaiscript::fun(
                [this](const std::vector<kpmr::riskprofile::NodeType> nodes)
                {
                    return this->kpmrNodeTypeAsChaiVar(nodes);
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

std::vector<kpmr::riskprofile::NodeType> riskprofile::KPMRReportGenerator::filterKPMRRiskProfileNodeXPath(
    const std::string &xpathExpr)
{
    std::vector<kpmr::riskprofile::NodeType> results;

    if (!kpmrRiskProfileTree)
    {
        throw std::runtime_error("KPMRRiskProfileTree is not set");
    }

    xercesc::DOMElement *root(static_cast<xercesc::DOMElement *>(kpmrRiskProfileTree->_node()));
    if (root == nullptr)
    {
        throw std::runtime_error("KPMRRiskProfileTree DOM root is null; parsing likely failed");
    }

    // Debug: Print root element name and namespace
    char* rootName = xercesc::XMLString::transcode(root->getLocalName());
    char* rootNS = xercesc::XMLString::transcode(root->getNamespaceURI());
    std::cout << "Root element: " << (rootName ? rootName : "<null>") 
              << ", Namespace: " << (rootNS ? rootNS : "<null>") << std::endl;
    xercesc::XMLString::release(&rootName);
    xercesc::XMLString::release(&rootNS);

    xercesc::DOMDocument *doc(root->getOwnerDocument());
    if (doc == nullptr)
    {
        throw std::runtime_error("KPMRRiskProfileTree DOM document is null");
    }

    xml_schema::dom::unique_ptr<XQillaNSResolver> resolver(
        (XQillaNSResolver *)doc->createNSResolver(root));

    // Set the namespace prefix for the kpmr namespace that
    // we can use reliably in XPath expressions regardless of
    // what is used in XML documents.
    //
    resolver->addNamespaceBinding(
        xsd::cxx::xml::string("kpmr").c_str(),
        xsd::cxx::xml::string("http://example.com/kpmr").c_str());
    
    // Debug: Check what namespace is bound
    std::cout << "Added namespace binding: kpmr -> http://example.com/kpmr" << std::endl;
    
    // Debug: Check first child element
    xercesc::DOMElement* firstChild = root->getFirstElementChild();
    if (firstChild) {
        char* childName = xercesc::XMLString::transcode(firstChild->getLocalName());
        char* childNS = xercesc::XMLString::transcode(firstChild->getNamespaceURI());
        std::cout << "First child element: " << (childName ? childName : "<null>") 
                  << ", Namespace: " << (childNS ? childNS : "<null>") << std::endl;
        xercesc::XMLString::release(&childName);
        xercesc::XMLString::release(&childNS);
    }
    
    xml_schema::dom::unique_ptr<const XQillaExpression> expr(
        static_cast<const XQillaExpression *>(
            doc->createExpression(
                xsd::cxx::xml::string(xpathExpr).c_str(),
                resolver.get())));

    xml_schema::dom::unique_ptr<xercesc::DOMXPathResult> r(
        expr->evaluate(doc, xercesc::DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

    std::cout << "Evaluating XPath: " << xpathExpr << std::endl;

    // Iterate over the result.
    //

    int matchCount = 0;
    while (r->iterateNext())
    {
        matchCount++;
        std::cout << "XPath matched node #" << matchCount << std::endl;

        const xercesc::DOMNode *n(r->getNodeValue());
        
        // Debug: Print node details
        if (n) {
            std::cout << "  Node type: " << n->getNodeType() << " (1=ELEMENT_NODE)" << std::endl;
            if (n->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
                char* nodeName = xercesc::XMLString::transcode(n->getLocalName());
                char* nodeNS = xercesc::XMLString::transcode(n->getNamespaceURI());
                std::cout << "  Element name: " << (nodeName ? nodeName : "<null>") 
                          << ", Namespace: " << (nodeNS ? nodeNS : "<null>") << std::endl;
                xercesc::XMLString::release(&nodeName);
                xercesc::XMLString::release(&nodeNS);
            }
        }

        // Obtain the object model node corresponding to
        // this DOM node.
        //
        kpmr::riskprofile::NodeType *p(
            static_cast<kpmr::riskprofile::NodeType *>(
                n->getUserData(xml_schema::dom::tree_node_key)));

        // Use the object model.
        //
        if (p != nullptr)
        {
            std::cout << "  Found mapped object: " << p->risiko_name() << std::endl;
            results.push_back(*p);
        }
        else
        {
            std::cout << "  WARNING: No tree_node_key user data on this node!" << std::endl;
        }
    }

    std::cout << "Total nodes matched: " << matchCount << ", Results returned: " << results.size() << std::endl;

    return results;
}

absl::Status riskprofile::KPMRReportGenerator::generateKPMRReport()
{
    if (!kpmrRiskProfileConf)
    {
        return absl::InvalidArgumentError("KPMRRiskProfileConf is not set");
    }

    std::cout << "Generating KPMR Report..." << std::endl;

    for (auto &it : this->kpmrRiskProfileConf->aggregationConfig())
    {
        std::cout << "Aggregation Config Risiko Name: " << it.risikoName() << std::endl;

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

absl::Status riskprofile::KPMRReportGenerator::writeKPMRRiskProfileConf(const std::string& outputPath)
{
    if (!kpmrRiskProfileConf)
    {
        return absl::InvalidArgumentError("KPMRRiskProfileConf is not set");
    }

    try
    {
        xml_schema::namespace_infomap map;
        map[""].name = "http://example.com/kpmr";
        map[""].schema = "./xsd/KPMRRiskProfileConf.xsd";

        // Serialize the data to XML file
        std::ofstream ofs(outputPath);
        if (!ofs.is_open())
        {
            std::cerr << "Error: Failed to open output file: " << outputPath << std::endl;
            return absl::InvalidArgumentError("Cannot open output file: " + outputPath);
        }

        kpmr::conf::riskprofile::RiskProfileConf_(ofs, *kpmrRiskProfileConf, map);
        ofs.close();

        std::cout << "Results written to: " << outputPath << std::endl;
        return absl::OkStatus();
    }
    catch (const std::exception& e)
    {
        return absl::InternalError(std::string("Failed to write output file: ") + e.what());
    }
}

std::vector<chaiscript::Boxed_Value> riskprofile::KPMRReportGenerator::kpmrNodeTypeAsChaiVar(std::vector<kpmr::riskprofile::NodeType> nodes)
{
    std::vector<chaiscript::Boxed_Value> boxedValues;

    for (auto &n : nodes)
    {
        std::cout << "Converting Node: " << n.risiko_name() << std::endl;
        chaiscript::Boxed_Value bv = chaiscript::Boxed_Value(n);

        std::map<std::string, chaiscript::Boxed_Value> nodeMap;
        std::string profileID = n.profile_id();
        
        nodeMap["profile_id"] = chaiscript::var(profileID);
        nodeMap["Computed_Weighted_Score"] = chaiscript::var(0.0);
        if (n.computed_weighted_score().present())
        {
            // Convert string to double
            double score = std::stod(n.computed_weighted_score().get());
            nodeMap["Computed_Weighted_Score"] = chaiscript::var(score);
        }

        boxedValues.push_back(chaiscript::var(nodeMap));
    }

    return boxedValues;
}

