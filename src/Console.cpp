#include "Console.hpp"

DragBar* DragBar::create() {
    auto dragBar = new DragBar();
    if (dragBar->init()) {
        dragBar->autorelease();
        return dragBar;
    }
    delete dragBar;
    return nullptr;
}

bool DragBar::init() {
    if (!CCLayerColor::init()) return false;
    setColor({0, 0, 0});
    setOpacity(127);
    setTouchEnabled(true);
    scheduleUpdate();

    CCLabelBMFont* logsLabel = CCLabelBMFont::create("Logs", "Consolas.fnt"_spr);
    logsLabel->setAnchorPoint({0, 0.5f});
    logsLabel->setScale(0.3f);
    logsLabel->setPosition({11, 4.5});

    m_resizeSprite = CCSprite::create("resize.png"_spr);
    m_resizeSprite->setAnchorPoint({1, 1});
    m_resizeSprite->setPosition(getContentSize());
    m_resizeSprite->setOpacity(64);

    addChild(m_resizeSprite);

    m_minimizeSprite = CCSprite::create("minimize.png"_spr);
    m_minimizeSprite->setAnchorPoint({0, 0.5f});
    m_minimizeSprite->setPosition({1, 4});
    m_minimizeSprite->setOpacity(64);

    addChild(m_minimizeSprite);

    schedule(schedule_selector(DragBar::resizeSchedule), 0.0083);

    addChild(logsLabel);

    if (Mod::get()->getSavedValue<bool>("isMinimized", false)) {
        m_minimized = true;
        m_minimizeSprite->setDisplayFrame(CCSprite::create("unminimize.png"_spr)->displayFrame());
        m_resizeSprite->setVisible(false);
    }

    return true;
}

class ScrollbarProMax : public geode::Scrollbar {
public:
    CCScale9Sprite* getTrack() {
        return m_track;
    }
    CCScale9Sprite* getThumb() {
        return m_thumb;
    }
};

void DragBar::setMinimized(bool minimized) {
    m_minimized = minimized;
    Console* console = static_cast<Console*>(m_nodeToMove);
    Mod::get()->setSavedValue("isMinimized", minimized);
    console->setMinimized(minimized);

    if (minimized) {
        m_minimizeSprite->setDisplayFrame(CCSprite::create("unminimize.png"_spr)->displayFrame());
        m_resizeSprite->setVisible(false);
    }
    else {
        m_minimizeSprite->setDisplayFrame(CCSprite::create("minimize.png"_spr)->displayFrame());
        m_resizeSprite->setVisible(true);
    }
}

void DragBar::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addPrioTargetedDelegate(this, -129,true);
}

void DragBar::setNodeToMove(CCNode* node) {
    m_nodeToMove = node;
}

void DragBar::setContentSize(const CCSize& size) {
    CCLayerColor::setContentSize(size);
    if (m_resizeSprite) {
        m_resizeSprite->setPosition(getContentSize());
    }
}

bool DragBar::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    CCPoint locationInView = touch->getLocation();
    CCPoint locationInNode = this->convertToNodeSpace(locationInView);
    m_expectedContentSize = m_nodeToMove->getContentSize();
    m_queuedSize = m_nodeToMove->getContentSize();

    float width = this->getContentSize().width - 20;

    if (m_minimized) width += 10;

    CCRect bounds = CCRect(10, 0, width, this->getContentSize().height);
    if (bounds.containsPoint(locationInNode)) {
        m_lastTouchPos = locationInView;
        m_dragging = true;
        return true;
    }

    if (!m_minimized) {
        CCRect resizeBounds = CCRect(this->getContentSize().width - 10, 0, 10, this->getContentSize().height);
        if (resizeBounds.containsPoint(locationInNode)) {
            m_lastTouchPos = locationInView;
            m_resizing = true;
            m_resizeSprite->setOpacity(127);
            return true;
        }
    }

    CCRect minimizeBounds = CCRect(0, 0, 10, this->getContentSize().height);
    if (minimizeBounds.containsPoint(locationInNode)) {
        m_minimizeSprite->setOpacity(127);
        setMinimized(!m_minimized);
        return true;
    }

    return false;
}

void DragBar::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_nodeToMove) return;
    
    CCPoint currentPos = touch->getLocation();
    CCPoint delta = currentPos - m_lastTouchPos;
    m_lastTouchPos = currentPos;

    CCPoint newPos = m_nodeToMove->getPosition() + delta;

    if (m_dragging) {
        CCNode* parent = m_nodeToMove->getParent();
        if (!parent)
            return;

        CCSize parentSize = parent->getContentSize();
        CCSize nodeSize = m_nodeToMove->getContentSize();
        CCPoint anchor = m_nodeToMove->getAnchorPoint();

        float minX = nodeSize.width * anchor.x;
        float maxX = parentSize.width - nodeSize.width * (1 - anchor.x);
        float minY = nodeSize.height * anchor.y;
        float maxY = parentSize.height - nodeSize.height * (1 - anchor.y);

        newPos.x = std::max(minX, std::min(newPos.x, maxX));
        newPos.y = std::max(minY, std::min(newPos.y, maxY));

        m_nodeToMove->setPosition(newPos);
    }
    if (m_resizing) {
        m_queuedSize = CCSize{m_expectedContentSize.width + delta.x, m_expectedContentSize.height + delta.y};
        CCSize winSize = CCDirector::get()->getWinSize();
        m_queuedSize.width = std::max(100.f, std::min(m_queuedSize.width, winSize.width));
        m_queuedSize.height = std::max(100.f, std::min(m_queuedSize.height, winSize.height));
        m_expectedContentSize = m_queuedSize;
    }
}

void DragBar::resizeSchedule(float dt) {
    if (m_resizing) {
        m_nodeToMove->setContentSize(m_queuedSize);
        CCNode* parent = m_nodeToMove->getParent();
        CCPoint pos = m_nodeToMove->getPosition();

        CCSize parentSize = parent->getContentSize();
        CCSize nodeSize = m_nodeToMove->getContentSize();
        CCPoint anchor = m_nodeToMove->getAnchorPoint();

        float minX = nodeSize.width * anchor.x;
        float maxX = parentSize.width - nodeSize.width * (1 - anchor.x);
        float minY = nodeSize.height * anchor.y;
        float maxY = parentSize.height - nodeSize.height * (1 - anchor.y);

        pos.x = std::max(minX, std::min(pos.x, maxX));
        pos.y = std::max(minY, std::min(pos.y, maxY));

        m_nodeToMove->setPosition(pos);
    }
}

void DragBar::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    m_dragging = false;
    m_resizing = false;
    m_resizeSprite->setOpacity(64);
    m_minimizeSprite->setOpacity(64);
}

void DragBar::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    m_dragging = false;
    m_resizing = false;
    m_resizeSprite->setOpacity(64);
    m_minimizeSprite->setOpacity(64);
}

Console* Console::s_instance = nullptr;

Console* Console::create() {
    auto console = new Console();
    if (console->init()) {
        console->autorelease();
        s_instance = console;
        return console;
    }
    delete console;
    return nullptr;
}

Console* Console::get() {
    return s_instance;
}

Console::~Console() {
    s_instance = nullptr;
}

bool Console::init() {
    if (!CCLayerColor::initWithColor({0, 0, 0, 220})) return false;
    this->setZOrder(100);
    m_blockMenu = CCMenu::create();
    m_blockMenu->ignoreAnchorPointForPosition(false);
    m_blockMenuItem = CCMenuItemSpriteExtra::create(CCNode::create(), this, nullptr);
    m_blockMenuItem->m_fSizeMult = 1;

    float posX = Mod::get()->getSavedValue<float>("posX", 20);
    float posY = Mod::get()->getSavedValue<float>("posY", 20);

    setPosition({posX, posY});

    float mainSizeWidth = Mod::get()->getSavedValue<float>("sizeWidth", 300);
    float mainSizeHeight = Mod::get()->getSavedValue<float>("sizeHeight", 150);

    CCSize mainSize = {mainSizeWidth, mainSizeHeight};

    setContentSize(mainSize);
    setAnchorPoint({0, 0});
    ignoreAnchorPointForPosition(false);

    m_blockMenu->setContentSize(mainSize);
    m_blockMenu->setPosition(0, 0);
    m_blockMenu->setAnchorPoint({0, 0});
    m_blockMenuItem->setContentSize(mainSize);
    m_blockMenuItem->setAnchorPoint({0, 0});

    m_blockMenu->addChild(m_blockMenuItem);

    addChild(m_blockMenu);

    m_scrollLayer = geode::ScrollLayer::create({0, 0, mainSize.width - 2, mainSize.height - 9});

    m_scrollLayer->m_contentLayer->setLayout(
        SimpleColumnLayout::create()
            ->setMainAxisDirection(AxisDirection::TopToBottom)
            ->setMainAxisAlignment(MainAxisAlignment::End)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setGap(2.5f)
    );
    m_scrollLayer->setPosition({1, 1});

    ScrollbarProMax* scrollbar = static_cast<ScrollbarProMax*>(geode::Scrollbar::create(m_scrollLayer));
    scrollbar->setTouchEnabled(false);
    scrollbar->setAnchorPoint({1, 0});
    scrollbar->setContentSize({4, mainSize.height - 10});
    scrollbar->setScaleX(0.75f);
    scrollbar->setPosition({getContentWidth(), 0});
    scrollbar->getTrack()->setOpacity(0);
    scrollbar->getThumb()->setOpacity(127);

    m_scrollbar = scrollbar;

    addChild(m_scrollLayer);
    addChild(scrollbar);

    m_dragBar = DragBar::create();
    m_dragBar->setNodeToMove(this);
    m_dragBar->setAnchorPoint({0, 1});
    m_dragBar->setPosition({0, getContentHeight()});
    m_dragBar->setContentSize({getContentWidth(), 8});
    m_dragBar->ignoreAnchorPointForPosition(false);
    m_dragBar->setZOrder(1);

    addChild(m_dragBar);

    handleTouchPriority(this);

    if (Mod::get()->getSavedValue<bool>("isMinimized", false)) {
        m_minimized = true;
        m_scrollbar->setVisible(false);
        m_scrollLayer->setVisible(false);
        m_blockMenu->setVisible(false);
        setContentSize({24, 8.5});
    }

    return true;
}

void Console::setMinimized(bool minimized) {
    m_minimized = minimized;
    m_scrollbar->setVisible(!minimized);
    m_scrollLayer->setVisible(!minimized);
    m_blockMenu->setVisible(!minimized);

    if (minimized) {
        setPosition({getPositionX(), getPositionY() + getContentSize().height - 8.5f});
        setContentSize({24, 8.5});
    }
    else {
        float mainSizeWidth = Mod::get()->getSavedValue<float>("sizeWidth", 300);
        float mainSizeHeight = Mod::get()->getSavedValue<float>("sizeHeight", 150);

        CCSize mainSize = {mainSizeWidth, mainSizeHeight};
        setContentSize(mainSize);
        setPosition({getPositionX(), getPositionY() - getContentSize().height + 8.5f});

        CCNode* parent = getParent();
        CCPoint pos = getPosition();

        CCSize parentSize = parent->getContentSize();
        CCSize nodeSize = getContentSize();
        CCPoint anchor = getAnchorPoint();

        float minX = nodeSize.width * anchor.x;
        float maxX = parentSize.width - nodeSize.width * (1 - anchor.x);
        float minY = nodeSize.height * anchor.y;
        float maxY = parentSize.height - nodeSize.height * (1 - anchor.y);

        pos.x = std::max(minX, std::min(pos.x, maxX));
        pos.y = std::max(minY, std::min(pos.y, maxY));

        setPosition(pos);
    }
}

int calculateOffset(Log log) {
    return 22 + log.threadName.size() + log.mod->getName().size();
}

void Console::pushLog(Log log) {
    if (m_scrollLayer->m_contentLayer->getChildrenCount() > 500) {
        static_cast<CCNode*>(m_scrollLayer->m_contentLayer->getChildren()->objectAtIndex(0))->removeFromParent();
    }

    std::vector<std::string> lines = geode::utils::string::split(log.message, "\n");
    std::vector<Log> logs;
    bool firstLine = true;;
    if (lines.size() > 1) {
        for (const std::string& line : lines) {
            logs.push_back({log.mod, log.severity, line, log.threadName, log.time, !firstLine, calculateOffset(log)});
            firstLine = false;
        }
    }
    else {
        logs.push_back(log);
    }

    for (const auto& log : logs) {
        if (m_scrollLayer->m_contentLayer->getChildrenCount() > 1) {
            CCNode* next = static_cast<CCNode*>(m_scrollLayer->m_contentLayer->getChildren()->objectAtIndex(m_scrollLayer->m_contentLayer->getChildrenCount() - 1));
            m_scrollLayer->m_contentLayer->insertAfter(createCell(log), next);
        }
        else {
            m_scrollLayer->m_contentLayer->addChild(createCell(log));
        }
    }
    m_scrollLayer->m_contentLayer->updateLayout();
    m_scrollLayer->m_contentLayer->setPosition(m_scrollLayer->m_contentLayer->getPosition());
}

CCNode* Console::createCell(Log log) {
    LogCell* logCell = LogCell::create(log, getContentSize());

    return logCell;
}

void Console::setContentSize(const CCSize& size) {
    CCLayerColor::setContentSize(size);
    if (m_scrollLayer) {
        m_scrollLayer->setContentSize({size.width - 2, size.height - 9});
        m_scrollLayer->m_contentLayer->setContentSize({size.width - 2, size.height - 9});
        for (LogCell* node : CCArrayExt<LogCell*>(m_scrollLayer->m_contentLayer->getChildren())) {
            node->resize(size);
        }
        m_scrollLayer->m_contentLayer->updateLayout();
        m_scrollLayer->m_contentLayer->setPosition(m_scrollLayer->m_contentLayer->getPosition());
    }
    if (m_dragBar) {
        m_dragBar->setPosition({0, size.height});
        m_dragBar->setContentSize({size.width, 8});
    }
    if (m_scrollbar) {
        m_scrollbar->setPosition({size.width, 0});
        m_scrollbar->setContentSize({4, size.height - 10});
    }
    if (m_blockMenu && m_blockMenuItem) {
        m_blockMenu->setContentSize(size);
        m_blockMenuItem->setContentSize(size);
    }
    if (!m_minimized && m_scrollLayer) {
        Mod::get()->setSavedValue("sizeWidth", getContentSize().width);
        Mod::get()->setSavedValue("sizeHeight", getContentSize().height);
    }
}

void Console::setPosition(const CCPoint& point) {
    CCLayerColor::setPosition(point);
    Mod::get()->setSavedValue("posX", getPositionX());
    Mod::get()->setSavedValue("posY", getPositionY());
}

LogCell* LogCell::create(Log log, CCSize size) {
    auto logCell = new LogCell();
    if (logCell->init(log, size)) {
        logCell->autorelease();
        return logCell;
    }
    delete logCell;
    return nullptr;
}

bool LogCell::init(Log log, CCSize size) {
    if (!CCNode::init()) return false;
    m_log = log;

    setAnchorPoint({0.f, 1.f});
    setContentSize({size.width-2, 8});

    RowLayout* layout = RowLayout::create();
    layout->setAxisAlignment(AxisAlignment::Start);
    layout->setCrossAxisAlignment(AxisAlignment::End);
    layout->setGrowCrossAxis(true);
    layout->setAutoScale(false);
    layout->setGap(0);
    setLayout(layout);

    ccColor3B color = {255, 255, 255};
    ccColor3B color2 = {255, 255, 255};

    switch (log.severity.m_value) {
        case Severity::Debug:
            color = {118, 118, 118};
            color2 = {188, 188, 188};
            break;
        case Severity::Info:
            color = {0, 135, 255};
            color2 = {228, 228, 228};
            break;
        case Severity::Warning:
            color = {255, 255, 175};
            color2 = {255, 255, 215};
            break;
        case Severity::Error:
            color = {231, 72, 86};
            color2 = {255, 215, 215};
            break;
        default:
            break;
    }

    std::string res;

    if (!log.newLine) {
        res = fmt::format("{:%H:%M:%S}", log.time);

        switch (log.severity.m_value) {
            case Severity::Debug:
                res += " DEBUG";
                break;
            case Severity::Info:
                res += " INFO ";
                break;
            case Severity::Warning:
                res += " WARN ";
                break;
            case Severity::Error:
                res += " ERROR";
                break;
            default:
                res += " ?????";
                break;
        }

        CCLabelBMFont* timeAndSeverityLabel = CCLabelBMFont::create(res.c_str(), "Consolas.fnt"_spr);
        timeAndSeverityLabel->setColor(color);
        timeAndSeverityLabel->setScale(0.3f);
        addChild(timeAndSeverityLabel);

        std::string threadAndMod;

        if (log.threadName.empty())
            threadAndMod += fmt::format(" [{}]: ", log.mod->getName());
        else
            threadAndMod += fmt::format(" [{}] [{}]: ", log.threadName, log.mod->getName());

        CCLabelBMFont* threadAndModLabel = CCLabelBMFont::create(threadAndMod.c_str(), "Consolas.fnt"_spr);
        threadAndModLabel->setColor(color2);
        threadAndModLabel->setScale(0.3f);
        addChild(threadAndModLabel);
    }
    else {
        res = std::string(log.offset, ' ');
        CCLabelBMFont* emptyLabel = CCLabelBMFont::create(res.c_str(), "Consolas.fnt"_spr);
        emptyLabel->setColor(color);
        emptyLabel->setScale(0.3f);
        addChild(emptyLabel);
    }

    std::vector<std::string> spaceSplit = geode::utils::string::split(log.message, " ");
    CCLabelBMFont* lastSpace;

    for (const std::string& string : spaceSplit) {
        CCLabelBMFont* label = CCLabelBMFont::create(string.c_str(), "Consolas.fnt"_spr);
        label->setScale(0.3f);
        label->setColor(color2);
       
        lastSpace = CCLabelBMFont::create(" ", "Consolas.fnt"_spr);
        lastSpace->setScale(0.3f);
        addChild(label);
        addChild(lastSpace);
    }
    lastSpace->removeFromParent();
    updateLayout();
    setContentHeight(getContentHeight() - 2);

    return true;
}

void LogCell::resize(CCSize size) {
    setContentSize({size.width-2, 8});
    updateLayout();
    setContentHeight(getContentHeight() - 2);
}