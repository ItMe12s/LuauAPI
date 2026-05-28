#include <cocos2d.h>

namespace cocos2d {
    bool CCObject::isEqual(CCObject const*) { return false; }
    void CCObject::acceptVisitor(CCDataVisitor&) {}
    void CCObject::encodeWithCoder(DS_Dictionary*) {}
    bool CCObject::canEncode() { return false; }
    int CCObject::getTag() const { return m_nTag; }
    void CCObject::setTag(int tag) { m_nTag = tag; }

    CCNode& CCNode::operator=(CCNode const&) { return *this; }

    void CCArray::addObject(CCObject*) {}

    ZipFile::~ZipFile() = default;
}
