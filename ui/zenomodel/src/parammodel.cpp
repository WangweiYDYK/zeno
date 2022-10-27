#include "parammodel.h"
#include "zassert.h"
#include "igraphsmodel.h"
#include <zenomodel/include/uihelper.h>


IParamModel::IParamModel(
        PARAM_CLASS paramType,
        IGraphsModel* pModel,
        const QPersistentModelIndex& subgIdx,
        const QPersistentModelIndex& nodeIdx,
        QObject* parent)
    : QAbstractItemModel(parent)
    , m_nodeIdx(nodeIdx)
    , m_subgIdx(subgIdx)
    , m_model(pModel)
    , m_class(paramType)
{
    Q_ASSERT(m_model);
}

IParamModel::~IParamModel()
{
}

bool IParamModel::getInputSockets(INPUT_SOCKETS& inputs)
{
    if (m_class == PARAM_INPUT)
    {
        for (int r = 0; r < rowCount(); r++)
        {
            QModelIndex idx = index(r, 0);

            QString name = nameFromRow(idx.row());
            auto itItem = m_items.find(name);
            ZASSERT_EXIT(itItem != m_items.end(), false);
            _ItemInfo& item = m_items[name];

            {
                INPUT_SOCKET inSocket;
                inSocket.info.control = item.ctrl;
                inSocket.info.defaultValue = item.pConst;
                inSocket.info.nodeid = m_nodeIdx.data(ROLE_OBJID).toString();
                inSocket.info.name = name;
                inSocket.info.type = item.type;
                inSocket.linkIndice = item.links;
                inputs.insert(name, inSocket);
            }
        }
        return true;
    }
    return false;
}

bool IParamModel::getOutputSockets(OUTPUT_SOCKETS& outputs)
{
    if (m_class != PARAM_OUTPUT)
        return false;

    for (int r = 0; r < rowCount(); r++)
    {
        QModelIndex idx = index(r, 0);

        QString name = nameFromRow(idx.row());
        auto itItem = m_items.find(name);
        ZASSERT_EXIT(itItem != m_items.end(), false);
        _ItemInfo& item = m_items[name];

        {
            OUTPUT_SOCKET outSocket;
            outSocket.info.control = item.ctrl;
            outSocket.info.defaultValue = item.pConst;
            outSocket.info.nodeid = m_nodeIdx.data(ROLE_OBJID).toString();
            outSocket.info.name = name;
            outSocket.info.type = item.type;
            outSocket.linkIndice = item.links;
            outputs.insert(name, outSocket);
        }
    }
    return true;
}

bool IParamModel::getParams(PARAMS_INFO& params)
{
    if (m_class != PARAM_PARAM)
        return false;

    for (int r = 0; r < rowCount(); r++)
    {
        QModelIndex idx = index(r, 0);

        QString name = nameFromRow(idx.row());
        auto itItem = m_items.find(name);
        ZASSERT_EXIT(itItem != m_items.end(), false);
        _ItemInfo& item = m_items[name];

        {
            PARAM_INFO paramInfo;
            paramInfo.bEnableConnect = false;
            paramInfo.control = item.ctrl;
            paramInfo.value = item.pConst;
            paramInfo.typeDesc = item.type;
            paramInfo.name = name;
            params.insert(name, paramInfo);
        }
    }
    return true;
}

void IParamModel::setInputSockets(const INPUT_SOCKETS& inputs)
{
    for (INPUT_SOCKET inSocket : inputs)
    {
        _ItemInfo item;
        item.ctrl = inSocket.info.control;
        item.name = inSocket.info.name;
        item.pConst = inSocket.info.defaultValue;
        item.type = inSocket.info.type;
        item.links = inSocket.linkIndice;
        appendRow(item.name, item.type, item.pConst, item.ctrl);
    }
}

void IParamModel::setParams(const PARAMS_INFO& params)
{
    for (PARAM_INFO paramInfo : params)
    {
        _ItemInfo item;
        item.ctrl = paramInfo.control;
        item.name = paramInfo.name;
        item.pConst = paramInfo.value;
        item.type = paramInfo.typeDesc;
        appendRow(item.name, item.type, item.pConst, item.ctrl);
    }
}

void IParamModel::setOutputSockets(const OUTPUT_SOCKETS& outputs)
{
    for (OUTPUT_SOCKET outSocket : outputs)
    {
        _ItemInfo item;
        item.ctrl = outSocket.info.control;
        item.name = outSocket.info.name;
        item.pConst = outSocket.info.defaultValue;
        item.type = outSocket.info.type;
        item.links = outSocket.linkIndice;
        appendRow(item.name, item.type, item.pConst, item.ctrl);
    }
}

QModelIndex IParamModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= rowCount())
        return QModelIndex();

    auto itRow = m_row2Key.find(row);
    ZASSERT_EXIT(itRow != m_row2Key.end(), QModelIndex());

    QString name = itRow.value();
    auto itItem = m_items.find(name);
    ZASSERT_EXIT(itItem != m_items.end(), QModelIndex());

    return createIndex(row, 0, nullptr);
}

QModelIndex IParamModel::index(const QString& name) const
{
    if (m_key2Row.find(name) == m_key2Row.end())
        return QModelIndex();

    int row = m_key2Row[name];
    return createIndex(row, 0, nullptr);
}

QString IParamModel::nameFromRow(int row) const
{
    QString name;
    auto itRow = m_row2Key.find(row);
    ZASSERT_EXIT(itRow != m_row2Key.end(), name);
    name = itRow.value();
    return name;
}

void IParamModel::clear()
{
    while (rowCount() > 0)
    {
        //safe to notify the remove msg.
        removeRows(0, 1);
    }
}

QModelIndex IParamModel::parent(const QModelIndex& child) const
{
    return QModelIndex();
}

int IParamModel::rowCount(const QModelIndex& parent) const
{
    return m_items.size();
}

int IParamModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

bool IParamModel::hasChildren(const QModelIndex& parent) const
{
    return false;
}

QVariant IParamModel::data(const QModelIndex& index, int role) const
{
    QString name = nameFromRow(index.row());
    auto itItem = m_items.find(name);
    ZASSERT_EXIT(itItem != m_items.end(), QModelIndex());
    const _ItemInfo& item = m_items[name];

    switch (role)
    {
    case Qt::DisplayRole:
    case ROLE_PARAM_NAME:   return item.name;
    case ROLE_PARAM_TYPE:   return item.type;
    case ROLE_PARAM_CTRL:   return item.ctrl;
    case ROLE_PARAM_VALUE:  return item.pConst;
    case ROLE_PARAM_LINKS:  return QVariant::fromValue(item.links);
    }
    return QVariant();
}

bool IParamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    QString name = nameFromRow(index.row());
    auto itItem = m_items.find(name);
    ZASSERT_EXIT(itItem != m_items.end(), false);
    _ItemInfo& item = m_items[name];
    QVariant oldValue;

    switch (role)
    {
        case ROLE_PARAM_NAME:
        {
            //rename
            QString oldName = item.name;
            QString newName = value.toString();
            oldValue = oldName;
            if (oldName == newName)
                return false;

            int row = m_key2Row[name];
            m_key2Row.remove(oldName);
            m_key2Row.insert(newName, row);
            m_row2Key[row] = newName;

            _ItemInfo newItem = item;
            newItem.name = newName;
            m_items.remove(oldName);
            m_items.insert(newName, newItem);

            if (m_class != PARAM_INPUT && m_class != PARAM_OUTPUT)
                return false;

            //resolve links.
            for (QPersistentModelIndex linkIdx : newItem.links)
            {
                //modify link info.
                QString outNode = linkIdx.data(ROLE_OUTNODE).toString();
                QString outSock = linkIdx.data(ROLE_OUTSOCK).toString();
                QString inNode = linkIdx.data(ROLE_INNODE).toString();
                QString inSock = linkIdx.data(ROLE_INSOCK).toString();

                if (m_class == PARAM_INPUT)
                {
                    ZASSERT_EXIT(inSock == oldName, false);
                    inSock = newName;
                }
                else
                {
                    ZASSERT_EXIT(outSock == oldName, false);
                    outSock = newName;
                }
                auto pLinkModel = m_model->linkModel();
                pLinkModel->setData(linkIdx, inNode, ROLE_INNODE);
                pLinkModel->setData(linkIdx, inSock, ROLE_INSOCK);
                pLinkModel->setData(linkIdx, outNode, ROLE_OUTNODE);
                pLinkModel->setData(linkIdx, outSock, ROLE_OUTSOCK);
            }
            break;
        }
        case ROLE_PARAM_TYPE:
        {
            if (item.type == value.toString())
                return false;
            oldValue = item.type;
            item.type = value.toString();
            break;
        }
        case ROLE_PARAM_CTRL:
        {
            if (item.ctrl == value.toInt())
                return false;
            oldValue = item.ctrl;
            item.ctrl = (PARAM_CONTROL)value.toInt();
            break;
        }
        case ROLE_PARAM_VALUE:
        {
            if (item.pConst == value)
                return false;
            oldValue = item.pConst;
            item.pConst = value;
            break;
        }
        default:
            return false;
    }
    emit dataChanged(index, index, QVector<int>{role});     //legacy signal
    emit mock_dataChanged(index, oldValue, role);   //custom signal.
    return true;
}

QVariant IParamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

bool IParamModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    return false;
}

QModelIndexList IParamModel::match(
    const QModelIndex& start,
    int role,
    const QVariant& value,
    int hits,
    Qt::MatchFlags flags) const
{
    return QModelIndexList();
}

QHash<int, QByteArray> IParamModel::roleNames() const
{
    return _base::roleNames();
}

bool IParamModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row);
    _removeRow(index(row, 0));
    endRemoveRows();
    return true;
}

bool IParamModel::_removeRow(const QModelIndex& index)
{
    //remove link from this param.
    QString name = nameFromRow(index.row());
    auto itItem = m_items.find(name);
    ZASSERT_EXIT(itItem != m_items.end(), false);
    _ItemInfo& item = m_items[name];

    if (m_class == PARAM_INPUT || m_class == PARAM_OUTPUT)
    {
        for (const QPersistentModelIndex& linkIdx : item.links)
        {
            m_model->removeLink(linkIdx, m_subgIdx, true);
        }
    }

    int row = index.row();
    for (int r = row + 1; r < rowCount(); r++)
    {
        const QString& key = m_row2Key[r];
        m_row2Key[r - 1] = key;
        m_key2Row[key] = r - 1;
    }
    m_row2Key.remove(rowCount() - 1);
    m_key2Row.remove(name);
    m_items.remove(name);

    m_model->markDirty();
    return true;
}

void IParamModel::insertRow(int row, const QString& name, const QString& type, const QVariant& deflValue, PARAM_CONTROL ctrl)
{
    beginInsertRows(QModelIndex(), row, row);
    bool ret = _insertRow(row, name, type, deflValue, ctrl);
    endInsertRows();
}

void IParamModel::appendRow(const QString& name, const QString& type, const QVariant& deflValue, PARAM_CONTROL ctrl)
{
    int n = rowCount();
    insertRow(n, name, type, deflValue, ctrl);
}

void IParamModel::setItem(const QModelIndex& idx, const QString& type, const QVariant& deflValue, PARAM_CONTROL ctrl)
{
    setData(idx, type, ROLE_PARAM_TYPE);
    setData(idx, deflValue, ROLE_PARAM_VALUE);
    setData(idx, ctrl, ROLE_PARAM_CTRL);
}

bool IParamModel::_insertRow(
    int row,
    const QString& name,
    const QString& type,
    const QVariant& deflValue,
    PARAM_CONTROL ctrl)
{
    ZASSERT_EXIT(m_items.find(name) == m_items.end(), false);
    int nRows = m_items.size();

    _ItemInfo item;
    item.name = name;
    item.ctrl = ctrl;
    item.pConst = deflValue;
    item.type = type;

    if (row == nRows)
    {
        //append
        m_items[name] = item;
        m_row2Key[nRows] = name;
        m_key2Row[name] = nRows;
    }
    else if (row < nRows)
    {
        auto itRow = m_row2Key.find(row);
        ZASSERT_EXIT(itRow != m_row2Key.end(), false);
        int nRows = rowCount();
        for (int r = nRows; r > row; r--)
        {
            const QString& key = m_row2Key[r - 1];
            m_row2Key[r] = key;
            m_key2Row[key] = r;
        }
        m_items[name] = item;
        m_row2Key[row] = name;
        m_key2Row[name] = row;
    }
    else
    {
        ZASSERT_EXIT(false, false);
    }

    m_model->markDirty();
    return true;
}