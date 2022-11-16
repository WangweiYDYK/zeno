#include "zenoproppanel.h"
#include "zenoapplication.h"
#include <zenomodel/include/graphsmanagment.h>
#include <zenomodel/include/modelrole.h>
#include <zenomodel/include/igraphsmodel.h>
#include <zenomodel/include/curvemodel.h>
#include "variantptr.h"
#include <zenoui/comctrl/zcombobox.h>
#include <zenoui/comctrl/zlabel.h>
#include <zenoui/style/zenostyle.h>
#include <zenoui/comctrl/gv/zenoparamwidget.h>
#include <zenoui/comctrl/zveceditor.h>
#include <zenomodel/include/uihelper.h>
#include <zenoui/comctrl/zexpandablesection.h>
#include <zenoui/comctrl/zlinewidget.h>
#include <zenoui/comctrl/zlineedit.h>
#include <zenoui/comctrl/ztextedit.h>
#include <zenoui/comctrl/zwidgetfactory.h>
#include "util/log.h"
#include "util/apphelper.h"
#include <zenomodel/include/curveutil.h>
#include <zenoui/comctrl/dialog/curvemap/zcurvemapeditor.h>
#include <zenoui/comctrl/dialog/zenoheatmapeditor.h>
#include "zenomainwindow.h"
#include <zenomodel/include/viewparammodel.h>
#include "../dialog/zeditparamlayoutdlg.h"
#include <zenoui/comctrl/zspinboxslider.h>


static QString initTabWidgetQss()
{
    return QString::fromUtf8("\
            QTabBar {\
                background-color: #22252C;\
                border-bottom: 1px solid rgb(24, 29, 33);\
                border-right: 0px;\
            }\
            \
            QTabBar::tab {\
                background: #22252C;\
                color: #737B85;\
                border-top: 1px solid rgb(24,29,33);\
                border-right: 1px solid rgb(24, 29, 33);\
                border-bottom: 1px solid rgb(24, 29, 33);\
                padding: 2px 16px 3px 16px;\
            }\
            \
            QTabBar::tab:top {\
                padding: 2px 16px 3px 16px;\
            }\
            \
            QTabBar::tab:selected {\
                background: #2D3239;\
                color: #C3D2DF;\
                border-bottom: 0px;\
                /*only way to disable the padding when selected*/\
                padding: 0px 16px 3px 16px;\
            }\
            \
            QTabBar::close-button {\
                image: url(:/icons/closebtn.svg);\
                subcontrol-position: right;\
            }\
            QTabBar::close-button:hover {\
                image: url(:/icons/closebtn_on.svg);\
        }");
}



ZenoPropPanel::ZenoPropPanel(QWidget* parent)
    : QWidget(parent)
    , m_bReentry(false)
    , m_tabWidget(nullptr)
{
    QVBoxLayout* pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    setLayout(pVLayout);
    setFocusPolicy(Qt::ClickFocus);

    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QColor("#2D3239"));
    setPalette(palette);
    setAutoFillBackground(true);
}

ZenoPropPanel::~ZenoPropPanel()
{
}

QSize ZenoPropPanel::sizeHint() const
{
    QSize sz = QWidget::sizeHint();
    return sz;
}

QSize ZenoPropPanel::minimumSizeHint() const
{
    QSize sz = QWidget::minimumSizeHint();
    return sz;
}

void ZenoPropPanel::clearLayout()
{
    setUpdatesEnabled(false);
    qDeleteAll(findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
    QVBoxLayout* pMainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    while (pMainLayout->count() > 0)
    {
        QLayoutItem* pItem = pMainLayout->itemAt(pMainLayout->count() - 1);
        pMainLayout->removeItem(pItem);
    }
    setUpdatesEnabled(true);
    m_tabWidget = nullptr;
    m_controls.clear();

    if (m_idx.isValid())
    {
        ViewParamModel* paramsModel = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
        if (paramsModel)
        {
            disconnect(paramsModel, &ViewParamModel::rowsInserted, this, &ZenoPropPanel::onViewParamInserted);
            disconnect(paramsModel, &ViewParamModel::rowsAboutToBeRemoved, this, &ZenoPropPanel::onViewParamAboutToBeRemoved);
            disconnect(paramsModel, &ViewParamModel::dataChanged, this, &ZenoPropPanel::onViewParamDataChanged);
        }
    }

    update();
}

void ZenoPropPanel::reset(IGraphsModel* pModel, const QModelIndex& subgIdx, const QModelIndexList& nodes, bool select)
{
    clearLayout();
    QVBoxLayout *pMainLayout = qobject_cast<QVBoxLayout *>(this->layout());

    if (!pModel || !select || nodes.isEmpty())
    {
        update();
        return;
    }

    m_subgIdx = subgIdx;
    m_idx = nodes[0];
    if (!m_idx.isValid())
        return;

    ViewParamModel* paramsModel = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
    if (!paramsModel)
        return;

    connect(paramsModel, &ViewParamModel::rowsInserted, this, &ZenoPropPanel::onViewParamInserted);
    connect(paramsModel, &ViewParamModel::rowsAboutToBeRemoved, this, &ZenoPropPanel::onViewParamAboutToBeRemoved);
    connect(paramsModel, &ViewParamModel::dataChanged, this, &ZenoPropPanel::onViewParamDataChanged);
    connect(pModel, &IGraphsModel::_rowsRemoved, this, [=]() {
        clearLayout();
    });
    connect(pModel, &IGraphsModel::modelClear, this, [=]() {
        clearLayout();
    });

    QStandardItem* root = paramsModel->invisibleRootItem();
    if (!root) return;

    QStandardItem* pRoot = root->child(0);
    if (!pRoot) return;

    m_tabWidget = new QTabWidget;
    m_tabWidget->setStyleSheet(initTabWidgetQss());
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(false);
    m_tabWidget->setMovable(false);
    m_tabWidget->setFont(QFont("Segoe UI Bold", 10));  //bug in qss font setting.
    m_tabWidget->tabBar()->setDrawBase(false);

    for (int i = 0; i < pRoot->rowCount(); i++)
    {
        QStandardItem* pTabItem = pRoot->child(i);
        syncAddTab(m_tabWidget, pTabItem, i);
    }

    pMainLayout->addWidget(m_tabWidget);
    pMainLayout->setSpacing(0);

    update();
}

void ZenoPropPanel::onViewParamInserted(const QModelIndex& parent, int first, int last)
{
    ZASSERT_EXIT(m_tabWidget);
    if (!m_idx.isValid())
        return;

    ViewParamModel* paramsModel = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
    ZASSERT_EXIT(paramsModel);

    if (!parent.isValid())
    {
        QStandardItem* root = paramsModel->invisibleRootItem();
        ZASSERT_EXIT(root);
        QStandardItem* pRoot = root->child(0);
        if (!pRoot) return;

        for (int i = 0; i < pRoot->rowCount(); i++)
        {
            QStandardItem* pTabItem = pRoot->child(i);
            syncAddTab(m_tabWidget, pTabItem, i);
        }
        return;
    }

    if (m_controls.isEmpty())
        return;

    QStandardItem* parentItem = paramsModel->itemFromIndex(parent);
    ZASSERT_EXIT(parentItem);
    QStandardItem* newItem = parentItem->child(first);
    int vType = newItem->data(ROLE_VPARAM_TYPE).toInt();
    const QString& name = newItem->data(ROLE_VPARAM_NAME).toString();
    if (vType == VPARAM_TAB)
    {
        syncAddTab(m_tabWidget, newItem, first);
    }
    else if (vType == VPARAM_GROUP)
    {
        ZASSERT_EXIT(parentItem->data(ROLE_VPARAM_TYPE) == VPARAM_TAB);
        const QString& tabName = parentItem->data(ROLE_VPARAM_NAME).toString();
        int idx = UiHelper::tabIndexOfName(m_tabWidget, tabName);
        QWidget* tabWid = m_tabWidget->widget(idx);
        QVBoxLayout* pTabLayout = qobject_cast<QVBoxLayout*>(tabWid->layout());
        if (pTabLayout == nullptr)
        {
            pTabLayout = new QVBoxLayout;
            pTabLayout->addStretch();
            tabWid->setLayout(pTabLayout);
        }
        syncAddGroup(pTabLayout, newItem, first);
    }
    else if (vType == VPARAM_PARAM)
    {
        ZASSERT_EXIT(parentItem->data(ROLE_VPARAM_TYPE) == VPARAM_GROUP);

        QStandardItem* pTabItem = parentItem->parent();
        ZASSERT_EXIT(pTabItem && pTabItem->data(ROLE_VPARAM_TYPE) == VPARAM_TAB);

        const QString& tabName = pTabItem->data(ROLE_VPARAM_NAME).toString();
        const QString& groupName = parentItem->data(ROLE_VPARAM_NAME).toString();
        const QString& paramName = name;

        ZExpandableSection* pGroupWidget = findGroup(tabName, groupName);
        if (!pGroupWidget)
            return;

        QGridLayout* pGroupLayout = qobject_cast<QGridLayout*>(pGroupWidget->contentLayout());
        ZASSERT_EXIT(pGroupLayout);
        if (pGroupWidget->title() == groupName)
        {
            QStandardItem* paramItem = parentItem->child(first);
            bool ret = syncAddControl(pGroupLayout, paramItem, first);
            if (ret)
            {
                pGroupWidget->updateGeo();
            }
        }
    }
}

bool ZenoPropPanel::syncAddControl(QGridLayout* pGroupLayout, QStandardItem* paramItem, int row)
{
    ZASSERT_EXIT(paramItem && pGroupLayout, false);
    QStandardItem* pGroupItem = paramItem->parent();
    ZASSERT_EXIT(pGroupItem, false);
    QStandardItem* pTabItem = pGroupItem->parent();
    ZASSERT_EXIT(pTabItem, false);

    const QString& tabName = pTabItem->data(ROLE_VPARAM_NAME).toString();
    const QString& groupName = pGroupItem->data(ROLE_VPARAM_NAME).toString();
    const QString& paramName = paramItem->data(ROLE_VPARAM_NAME).toString();
    const QVariant& val = paramItem->data(ROLE_PARAM_VALUE);
    PARAM_CONTROL ctrl = (PARAM_CONTROL)paramItem->data(ROLE_PARAM_CTRL).toInt();
    const QString& typeDesc = paramItem->data(ROLE_PARAM_TYPE).toString();
    CONTROL_PROPERTIES pros = paramItem->data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();

    QPersistentModelIndex perIdx(paramItem->index());

    Callback_EditFinished cbEditFinish = [=](QVariant newValue) {
        //trick implementation:
        //todo: api scoped and transaction: undo/redo problem.
        if (!m_idx.isValid() || !perIdx.isValid())
            return;
        paramItem->setData(newValue, ROLE_PARAM_VALUE);
    };

    auto cbSwitch = [=](bool bOn) {
        zenoApp->getMainWindow()->setInDlgEventLoop(bOn);   //deal with ubuntu dialog slow problem when update viewport.
    };

    QWidget* pControl = zenoui::createWidget(val, ctrl, typeDesc, cbEditFinish, cbSwitch, pros);
    //if (!pControl)
    //    return false;

    QLabel* pLabel = new QLabel(paramName);
    pLabel->setProperty("cssClass", "proppanel");

    pGroupLayout->addWidget(pLabel, row, 0, Qt::AlignLeft);
    if (pControl)
        pGroupLayout->addWidget(pControl, row, 1);

    _PANEL_CONTROL panelCtrl;
    panelCtrl.controlLayout = pGroupLayout;
    panelCtrl.pLabel = pLabel;
    panelCtrl.m_viewIdx = perIdx;
    panelCtrl.pControl = pControl;

    m_controls[tabName][groupName][paramName] = panelCtrl;
    return true;
}

bool ZenoPropPanel::syncAddGroup(QVBoxLayout* pTabLayout, QStandardItem* pGroupItem, int row)
{
    const QString& groupName = pGroupItem->data(Qt::DisplayRole).toString();
    bool bCollaspe = pGroupItem->data(ROLE_VPARAM_COLLASPED).toBool();
    ZExpandableSection* pGroupWidget = new ZExpandableSection(groupName);
    pGroupWidget->setObjectName(groupName);
    pGroupWidget->setCollasped(bCollaspe);
    QGridLayout* pLayout = new QGridLayout;
    pLayout->setContentsMargins(10, 15, 0, 15);
    pLayout->setColumnStretch(0, 1);
    pLayout->setColumnStretch(1, 3);
    pLayout->setSpacing(10);
    for (int k = 0; k < pGroupItem->rowCount(); k++)
    {
        QStandardItem* paramItem = pGroupItem->child(k);
        syncAddControl(pLayout, paramItem, k);
    }
    pGroupWidget->setContentLayout(pLayout);
    pTabLayout->addWidget(pGroupWidget);

    connect(pGroupWidget, &ZExpandableSection::stateChanged, this, [=](bool bCollasped) {
        if (!m_idx.isValid())
            return;
        pGroupItem->setData(bCollasped, ROLE_VPARAM_COLLASPED);
    });
    return true;
}

bool ZenoPropPanel::syncAddTab(QTabWidget* pTabWidget, QStandardItem* pTabItem, int row)
{
    const QString& tabName = pTabItem->data(Qt::DisplayRole).toString();

    QWidget* pTabWid = new QWidget;
    QVBoxLayout* pTabLayout = new QVBoxLayout;
    pTabLayout->setContentsMargins(QMargins(0, 0, 0, 0));
    pTabLayout->setSpacing(0);

    for (int j = 0; j < pTabItem->rowCount(); j++)
    {
        QStandardItem* pGroupItem = pTabItem->child(j);
        syncAddGroup(pTabLayout, pGroupItem, j);
    }

    pTabLayout->addStretch();
    pTabWid->setLayout(pTabLayout);
    pTabWidget->insertTab(row, pTabWid, tabName);
    return true;
}

void ZenoPropPanel::onViewParamAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    if (m_controls.isEmpty() || !m_idx.isValid())
        return;

    ViewParamModel* paramsModel = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
    ZASSERT_EXIT(paramsModel);

    QStandardItem* parentItem = paramsModel->itemFromIndex(parent);
    if (parentItem == nullptr)
    {
        //clear all
        while (m_tabWidget->count() > 0)
        {
            QWidget* wid = m_tabWidget->widget(0);
            m_tabWidget->removeTab(0);
            delete wid;
        }
        return;
    }

    QStandardItem* removeItem = parentItem->child(first);
    int vType = removeItem->data(ROLE_VPARAM_TYPE).toInt();
    const QString& name = removeItem->data(ROLE_VPARAM_NAME).toString();

    if (VPARAM_TAB == vType)
    {
        int idx = UiHelper::tabIndexOfName(m_tabWidget, name);
        m_tabWidget->removeTab(idx);
    }
    else if (VPARAM_GROUP == vType)
    {
        ZASSERT_EXIT(parentItem->data(ROLE_VPARAM_TYPE) == VPARAM_TAB);
        const QString& tabName = parentItem->data(ROLE_VPARAM_NAME).toString();
        int idx = UiHelper::tabIndexOfName(m_tabWidget, tabName);
        QWidget* tabWid = m_tabWidget->widget(idx);
        QVBoxLayout* pTabLayout = qobject_cast<QVBoxLayout*>(tabWid->layout());
        for (int i = 0; i < pTabLayout->count(); i++)
        {
            QLayoutItem* pLayoutItem = pTabLayout->itemAt(i);
            if (ZExpandableSection* pGroup = qobject_cast<ZExpandableSection*>(pLayoutItem->widget()))
            {
                if (pGroup->title() == name)
                {
                    delete pGroup;
                    pTabLayout->removeItem(pLayoutItem);
                    break;
                }
            }
        }
    }
    else if (VPARAM_PARAM == vType)
    {
        ZASSERT_EXIT(parentItem->data(ROLE_VPARAM_TYPE) == VPARAM_GROUP);

        QStandardItem* pTabItem = parentItem->parent();
        ZASSERT_EXIT(pTabItem && pTabItem->data(ROLE_VPARAM_TYPE) == VPARAM_TAB);

        const QString& tabName = pTabItem->data(ROLE_VPARAM_NAME).toString();
        const QString& groupName = parentItem->data(ROLE_VPARAM_NAME).toString();
        const QString& paramName = name;

        ZExpandableSection* pGroupWidget = findGroup(tabName, groupName);
        if (!pGroupWidget)  return;

        QGridLayout* pGroupLayout = qobject_cast<QGridLayout*>(pGroupWidget->contentLayout());
        ZASSERT_EXIT(pGroupLayout);
        if (pGroupWidget->title() == groupName)
        {
            _PANEL_CONTROL& ctrl = m_controls[tabName][groupName][paramName];
            if (ctrl.controlLayout)
            {
                QGridLayout* pGridLayout = qobject_cast<QGridLayout*>(ctrl.controlLayout);
                ZASSERT_EXIT(pGridLayout);

                ctrl.controlLayout->removeWidget(ctrl.pControl);
                ctrl.controlLayout->removeWidget(ctrl.pLabel);
                delete ctrl.pControl;
                delete ctrl.pLabel;
                m_controls[tabName][groupName].remove(paramName);
            }
        }
    }
}

void ZenoPropPanel::onViewParamDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (topLeft.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM || !m_idx.isValid() || m_controls.isEmpty())
        return;

    ViewParamModel* paramsModel = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
    ZASSERT_EXIT(paramsModel);

    QStandardItem* paramItem = paramsModel->itemFromIndex(topLeft);
    ZASSERT_EXIT(paramItem);

    QStandardItem* groupItem = paramItem->parent();
    ZASSERT_EXIT(groupItem);

    QStandardItem* tabItem = groupItem->parent();
    ZASSERT_EXIT(tabItem);

    int role = roles[0];
    const QString& tabName = tabItem->data(ROLE_VPARAM_NAME).toString();
    const QString& groupName = groupItem->data(ROLE_VPARAM_NAME).toString();

    PANEL_GROUP& group = m_controls[tabName][groupName];

    for (int r = topLeft.row(); r <= bottomRight.row(); r++)
    {
        QStandardItem* param = groupItem->child(r);

        if (role == ROLE_VPARAM_NAME)
        {
            for (auto it = group.begin(); it != group.end(); it++)
            {
                if (it->second.m_viewIdx == param->index())
                {
                    const QString& newName = it->second.m_viewIdx.data(ROLE_VPARAM_NAME).toString();
                    it->second.pLabel->setText(newName);
                    it->first = newName;
                    break;
                }
            }
        }
        else if (role == ROLE_PARAM_CTRL)
        {
            const QString& paramName = param->data(ROLE_VPARAM_NAME).toString();
            _PANEL_CONTROL& ctrl = group[paramName];
            QGridLayout* pGridLayout = qobject_cast<QGridLayout*>(ctrl.controlLayout);
            ZASSERT_EXIT(pGridLayout);

            ctrl.controlLayout->removeWidget(ctrl.pControl);
            ctrl.controlLayout->removeWidget(ctrl.pLabel);
            delete ctrl.pControl;
            delete ctrl.pLabel;

            int row = group.keys().indexOf(paramName, 0);
            syncAddControl(pGridLayout, param, row);
        }
        else if (role == ROLE_PARAM_VALUE)
        {
            const QString& paramName = param->data(ROLE_VPARAM_NAME).toString();
            const QVariant& value = param->data(ROLE_PARAM_VALUE);
            _PANEL_CONTROL& ctrl = m_controls[tabName][groupName][paramName];
            if (QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(ctrl.pControl))
            {
                pLineEdit->setText(value.toString());
            }
            else if (QComboBox* pCombobox = qobject_cast<QComboBox*>(ctrl.pControl))
            {
                pCombobox->setCurrentText(value.toString());
            }
            else if (QTextEdit* pTextEidt = qobject_cast<QTextEdit*>(ctrl.pControl))
            {
                pTextEidt->setText(value.toString());
            }
            else if (ZVecEditor* pVecEdit = qobject_cast<ZVecEditor*>(ctrl.pControl))
            {
                pVecEdit->setVec(value.value<UI_VECTYPE>(), pVecEdit->isFloat());
            }
            else if (QCheckBox* pCheckbox = qobject_cast<QCheckBox*>(ctrl.pControl))
            {
                pCheckbox->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
            }
            else if (QSlider* pSlider = qobject_cast<QSlider*>(ctrl.pControl))
            {
                pSlider->setValue(value.toInt());
            }
            else if (QSpinBox* pSpinBox = qobject_cast<QSpinBox*>(ctrl.pControl))
            {
                pSpinBox->setValue(value.toInt());
            }
            else if (ZSpinBoxSlider* pSpinSlider = qobject_cast<ZSpinBoxSlider*>(ctrl.pControl))
            {
                pSpinSlider->setValue(value.toInt());
            }
            //...
        }
    }
}

ZExpandableSection* ZenoPropPanel::findGroup(const QString& tabName, const QString& groupName)
{
    QWidget* tabWid = m_tabWidget->widget(UiHelper::tabIndexOfName(m_tabWidget, tabName));
    ZASSERT_EXIT(tabWid, nullptr);
    auto lst = tabWid->findChildren<ZExpandableSection*>(QString(), Qt::FindDirectChildrenOnly);
    for (ZExpandableSection* pGroupWidget : lst)
    {
        if (pGroupWidget->title() == groupName)
        {
            return pGroupWidget;
        }
    }
    return nullptr;
}

void ZenoPropPanel::onSettings()
{
    QMenu* pMenu = new QMenu(this);
    pMenu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* pEditLayout = new QAction(tr("Edit Parameter Layout"));
    pMenu->addAction(pEditLayout);
    connect(pEditLayout, &QAction::triggered, [=]() {
        if (!m_idx.isValid())
            return;

        ViewParamModel* viewParams = QVariantPtr<ViewParamModel>::asPtr(m_idx.data(ROLE_VIEWPARAMS));
        ZASSERT_EXIT(viewParams);

        ZEditParamLayoutDlg dlg(viewParams, false, m_idx, this);
        dlg.exec();
    });
    pMenu->exec(QCursor::pos());
}
