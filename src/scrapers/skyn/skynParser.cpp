
#include <gumbo.h>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/skyn/skynModel.hpp>
#include <scrapers/skyn/skynScraper.hpp>

namespace HT::SKYN {

void parseSkynProperty(GumboNode *node, SkynProperty *p) {
  if (!node || node->type != GUMBO_NODE_ELEMENT)
    return;

  const char *classAttr = getClassAttr(node);
  if (classAttr) {
    std::string classStr = classAttr;

    if (classStr.find("ogn_headline") != std::string::npos)
      p->ogn_headline = getNodeText(node);
    else if (classStr.find("ogn_adress") != std::string::npos)
      p->ogn_address = getNodeText(node);
    else if (classStr.find("prop-size") != std::string::npos)
      p->prop_size = getNodeText(node->parent); // text is in sibling <br>
    else if (classStr.find("prop-ground-size") != std::string::npos)
      p->prop_ground_size = getNodeText(node->parent);
    else if (classStr.find("prop-bedrooms") != std::string::npos)
      p->prop_bedrooms = getNodeText(node->parent);
    else if (classStr.find("prop-floors") != std::string::npos)
      p->prop_floors = getNodeText(node->parent);
    else if (classStr.find("prop-buildyear") != std::string::npos)
      p->prop_buildYear = getNodeText(node->parent);
    else if (classStr.find("latestoffer") != std::string::npos)
      p->prop_latestOffer = getNodeText(node);
    else if (classStr.find("validto") != std::string::npos)
      p->prop_validToDate = getNodeText(node);
    else if (classStr.find("listprice") != std::string::npos)
      p->prop_listPrice = getNodeText(node);
  }

  // Handle the image from ogn_thumb
  if (classAttr &&
      std::string(classAttr).find("ogn_thumb") != std::string::npos) {
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      if (child->type == GUMBO_NODE_ELEMENT &&
          child->v.element.tag == GUMBO_TAG_IMG) {
        const char *src = getAttribute(&child->v.element.attributes, "src");
        if (src) {
          p->img = std::string("https://www.skyn.fo") + src; // fix relative URL
        }
      }
    }
  }

  // Recurse
  if (node->type == GUMBO_NODE_ELEMENT) {
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      parseSkynProperty(child, p);
    }
  }
}

std::vector<RawProperty>
mapBetriToRawProperty(std::vector<RawProperty> &rawProperties,
                      std::vector<SkynProperty> &skynProperties,
                      PropertyType propType) {
  for (auto &prop : skynProperties) {
    RawProperty p;
    prop.type = PropertyManager::propertyTypeToString(propType);
    prop.id = PropertyManager::cleanId(prop.ogn_headline + prop.ogn_address);

    p.id = prop.id;
    p.website = prop.website;
    p.address = prop.ogn_headline;
    p.type = prop.type;
    p.city = prop.ogn_address;
    p.price = prop.prop_listPrice;
    p.latestOffer = prop.prop_latestOffer;
    p.validDate = prop.prop_validToDate;
    p.buildingSize = prop.prop_size;
    p.landSize = prop.prop_ground_size;
    p.room = prop.prop_bedrooms;
    p.floor = prop.prop_floors;
    p.img = prop.img;
    p.agent = PropertyManager::propertyAgentToString(RealEstateAgent::Skyn);
    rawProperties.push_back(p);
  }
  return rawProperties;
}

void findSkynProperties(GumboNode *node, std::vector<SkynProperty> &results) {
  if (!node)
    return;

  if (node->type == GUMBO_NODE_ELEMENT) {
    const char *classAttr = getClassAttr(node);
    if (classAttr && std::string(classAttr).find("ogn") != std::string::npos) {
      SkynProperty prop;
      prop.website = "Skyn";

      parseSkynProperty(node, &prop);

      // Make a unique-ish id (you can clean this with your cleanId function)
      prop.id = PropertyManager::cleanId(prop.ogn_headline + prop.ogn_address);

      results.push_back(prop);
      return; // Skip deeper traversal — already parsed this block
    }
    // Continue looking through children
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      findSkynProperties(static_cast<GumboNode *>(children->data[i]), results);
    }
  }
}

std::vector<RawProperty> parseWithGumboSkyn(std::string html,
                                            PropertyType propType) {
  std::vector<SkynProperty> props;
  std::vector<RawProperty> rawProps;
  GumboOutput *output = gumbo_parse(html.c_str());
  findSkynProperties(output->root, props);
  gumbo_destroy_output(&kGumboDefaultOptions, output);
  mapBetriToRawProperty(rawProps, props, propType);
  return rawProps;
}

} // namespace HT::SKYN
