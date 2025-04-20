
#include <gumbo.h>
#include <regex>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/skyn/skynModel.hpp>
#include <scrapers/skyn/skynScraper.hpp>

namespace HT::SKYN {

bool hasExactClass(const char *classAttr, const std::string &targetClass) {
  if (!classAttr)
    return false;
  std::istringstream iss(classAttr);
  std::string token;
  while (iss >> token) {
    if (token == targetClass)
      return true;
  }
  return false;
}

void parseSkynProperty(GumboNode *node, SkynProperty *p) {
  if (!node || node->type != GUMBO_NODE_ELEMENT)
    return;

  const char *classAttr = getClassAttr(node);
  std::string cls = classAttr ? classAttr : "";

  /* ---------- 1. pick up the picture wherever it is -------------- */
  if (node->v.element.tag == GUMBO_TAG_IMG) {
    const char *src = getAttribute(&node->v.element.attributes, "src");
    if (src && std::strstr(src, "/admin/public/getimage") != nullptr) {
      std::string imgstring = static_cast<std::string>(src);
      p->img = "https://www.skyn.fo" + imgstring; // absolute URL already
    }
  }

  /* ---------- 2. the ordinary class‑based fields ----------------- */
  if (!cls.empty()) {
    if (cls.find("ogn_headline") != std::string::npos)
      p->ogn_headline = getNodeText(node);
    else if (cls.find("ogn_adress") != std::string::npos)
      p->ogn_address = getNodeText(node);
    else if (cls.find("prop-size") != std::string::npos)
      p->prop_size = getNodeText(node->parent);
    else if (cls.find("prop-ground") != std::string::npos)
      p->prop_ground_size = getNodeText(node->parent);
    else if (cls.find("prop-bedrooms") != std::string::npos)
      p->prop_bedrooms = getNodeText(node->parent);
    else if (cls.find("prop-floors") != std::string::npos)
      p->prop_floors = getNodeText(node->parent);
    else if (cls.find("prop-buildyear") != std::string::npos)
      p->prop_buildYear = getNodeText(node->parent);
    else if (hasExactClass(classAttr, "latestoffer"))
      p->prop_latestOffer = getNodeText(node);
    else if (cls.find("validto") != std::string::npos)
      p->prop_validToDate = getNodeText(node);
    else if (hasExactClass(classAttr, "listprice"))
      p->prop_listPrice = getNodeText(node);
  }

  /* 3. recurse */
  GumboVector *ch = &node->v.element.children;
  for (unsigned int i = 0; i < ch->length; ++i)
    parseSkynProperty(static_cast<GumboNode *>(ch->data[i]), p);
}

std::vector<RawProperty>
mapBetriToRawProperty(std::vector<RawProperty> &rawProperties,
                      std::vector<SkynProperty> &skynProperties,
                      PropertyType propType) {
  for (auto &sp : skynProperties) {
    RawProperty rp;
    sp.type = PropertyManager::propertyTypeToString(propType);
    sp.id = PropertyManager::cleanId(sp.ogn_headline + sp.ogn_address);

    rp.id = sp.id;
    rp.website = sp.website;
    rp.address = sp.ogn_headline;
    rp.type = sp.type;
    rp.city = sp.ogn_address;
    rp.price = sp.prop_listPrice;
    rp.latestOffer = sp.prop_latestOffer;
    rp.validDate = sp.prop_validToDate;
    rp.buildingSize = sp.prop_size;
    rp.landSize = sp.prop_ground_size;
    rp.room = sp.prop_bedrooms;
    rp.floor = sp.prop_floors;
    rp.img = sp.img;
    rp.agent = PropertyManager::propertyAgentToString(RealEstateAgent::Skyn);
    rawProperties.push_back(rp);
  }
  return rawProperties;
}

// helper: true if the class attribute contains the *word* "ogn"
// (not just the substring)
static bool classHasWordOgn(const char *cls) {
  if (!cls)
    return false;
  // word boundaries: start/end or whitespace around "ogn"
  static const std::regex re(R"((^|\s)ogn(\s|$))");
  return std::regex_search(cls, re);
}

void findSkynProperties(GumboNode *node, std::vector<SkynProperty> &results) {
  if (!node || node->type != GUMBO_NODE_ELEMENT)
    return;

  const char *cls = getClassAttr(node);

  /* ------------------------------------------------------------
     1. did we hit the outer wrapper  <div class="… ognlist"> ... ?
     ------------------------------------------------------------ */
  if (cls && std::strstr(cls, "ognlist") != nullptr) {

    GumboVector *kids = &node->v.element.children;

    for (unsigned i = 0; i < kids->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(kids->data[i]);

      /* look only at element nodes, skip the <script> tag */
      if (child->type != GUMBO_NODE_ELEMENT)
        continue;

      const char *childCls = getClassAttr(child);
      if (!classHasWordOgn(childCls))
        continue; // not a property card

      /* ---------------------------------------------------
         2. parse one property card
         --------------------------------------------------- */
      SkynProperty prop;
      prop.website = "Skyn";

      parseSkynProperty(child, &prop);

      prop.id = PropertyManager::cleanId(prop.ogn_headline + prop.ogn_address);
      results.push_back(std::move(prop));
    }
    return; // we’ve handled all properties; no recursion
  }

  /* ------------------------------------------------------------
     Otherwise keep searching for the wrapper further down
     ------------------------------------------------------------ */
  GumboVector *kids = &node->v.element.children;
  for (unsigned i = 0; i < kids->length; ++i)
    findSkynProperties(static_cast<GumboNode *>(kids->data[i]), results);
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
