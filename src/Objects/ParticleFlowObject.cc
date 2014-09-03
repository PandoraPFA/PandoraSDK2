/**
 *  @file   PandoraSDK/src/Objects/ParticleFlowObject.cc
 * 
 *  @brief  Implementation of the particle flow object class.
 * 
 *  $Log: $
 */

#include "Objects/Cluster.h"
#include "Objects/OrderedCaloHitList.h"
#include "Objects/ParticleFlowObject.h"
#include "Objects/Track.h"

namespace pandora
{

ParticleFlowObject::ParticleFlowObject(const PandoraContentApi::ParticleFlowObject::Parameters &parameters) :
    m_particleId(parameters.m_particleId.Get()),
    m_charge(parameters.m_charge.Get()),
    m_mass(parameters.m_mass.Get()),
    m_energy(parameters.m_energy.Get()),
    m_momentum(parameters.m_momentum.Get()),
    m_trackList(parameters.m_trackList),
    m_clusterList(parameters.m_clusterList),
    m_vertexList(parameters.m_vertexList)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

TrackAddressList ParticleFlowObject::GetTrackAddressList() const
{
    TrackAddressList trackAddressList;

    for (TrackList::const_iterator iter = m_trackList.begin(), iterEnd = m_trackList.end(); iter != iterEnd; ++iter)
    {
        trackAddressList.push_back((*iter)->GetParentTrackAddress());
    }

    return trackAddressList;
}

//------------------------------------------------------------------------------------------------------------------------------------------

ClusterAddressList ParticleFlowObject::GetClusterAddressList() const
{
    ClusterAddressList clusterAddressList;

    for (ClusterList::const_iterator iter = m_clusterList.begin(), iterEnd = m_clusterList.end(); iter != iterEnd; ++iter)
    {
        CaloHitAddressList caloHitAddressList;

        OrderedCaloHitList orderedCaloHitList((*iter)->GetOrderedCaloHitList());
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, orderedCaloHitList.Add((*iter)->GetIsolatedCaloHitList()));

        for (OrderedCaloHitList::const_iterator layerIter = orderedCaloHitList.begin(), layerIterEnd = orderedCaloHitList.end();
            layerIter != layerIterEnd; ++layerIter)
        {
            for (CaloHitList::const_iterator hitIter = layerIter->second->begin(), hitIterEnd = layerIter->second->end();
                hitIter != hitIterEnd; ++hitIter)
            {
                caloHitAddressList.push_back((*hitIter)->GetParentCaloHitAddress());
            }
        }

        clusterAddressList.push_back(caloHitAddressList);
    }

    return clusterAddressList;
}

//------------------------------------------------------------------------------------------------------------------------------------------

template <>
StatusCode ParticleFlowObject::AddToPfo(Cluster *pCluster)
{
    if (!m_clusterList.insert(pCluster).second)
        return STATUS_CODE_ALREADY_PRESENT;

    return STATUS_CODE_SUCCESS;
}

template <>
StatusCode ParticleFlowObject::AddToPfo(Track *pTrack)
{
    if (!m_trackList.insert(pTrack).second)
        return STATUS_CODE_ALREADY_PRESENT;

    return STATUS_CODE_SUCCESS;
}

template <>
StatusCode ParticleFlowObject::AddToPfo(Vertex *pVertex)
{
    if (!m_vertexList.insert(pVertex).second)
        return STATUS_CODE_ALREADY_PRESENT;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

template <>
StatusCode ParticleFlowObject::RemoveFromPfo(Cluster *pCluster)
{
    ClusterList::iterator iter = m_clusterList.find(pCluster);

    if (m_clusterList.end() == iter)
        return STATUS_CODE_NOT_FOUND;

    m_clusterList.erase(iter);
    return STATUS_CODE_SUCCESS;
}

template <>
StatusCode ParticleFlowObject::RemoveFromPfo(Track *pTrack)
{
    TrackList::iterator iter = m_trackList.find(pTrack);

    if (m_trackList.end() == iter)
        return STATUS_CODE_NOT_FOUND;

    m_trackList.erase(iter);
    return STATUS_CODE_SUCCESS;
}

template <>
StatusCode ParticleFlowObject::RemoveFromPfo(Vertex *pVertex)
{
    VertexList::iterator iter = m_vertexList.find(pVertex);

    if (m_vertexList.end() == iter)
        return STATUS_CODE_NOT_FOUND;

    m_vertexList.erase(iter);
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ParticleFlowObject::AddParent(ParticleFlowObject *const pPfo)
{
    if (NULL == pPfo)
        return STATUS_CODE_INVALID_PARAMETER;

    if (!m_parentPfoList.insert(pPfo).second)
        return STATUS_CODE_ALREADY_PRESENT;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ParticleFlowObject::AddDaughter(ParticleFlowObject *const pPfo)
{
    if (NULL == pPfo)
        return STATUS_CODE_INVALID_PARAMETER;

    if (!m_daughterPfoList.insert(pPfo).second)
        return STATUS_CODE_ALREADY_PRESENT;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ParticleFlowObject::RemoveParent(ParticleFlowObject *const pPfo)
{
    PfoList::const_iterator iter = m_parentPfoList.find(pPfo);

    if (m_parentPfoList.end() == iter)
        return STATUS_CODE_NOT_FOUND;

    m_parentPfoList.erase(iter);
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ParticleFlowObject::RemoveDaughter(ParticleFlowObject *const pPfo)
{
    PfoList::const_iterator iter = m_daughterPfoList.find(pPfo);

    if (m_daughterPfoList.end() == iter)
        return STATUS_CODE_NOT_FOUND;

    m_daughterPfoList.erase(iter);
    return STATUS_CODE_SUCCESS;
}

} // namespace pandora
