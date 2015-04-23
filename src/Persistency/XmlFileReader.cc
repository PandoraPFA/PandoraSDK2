/**
 *  @file   PandoraSDK/src/Persistency/XmlFileReader.cc
 * 
 *  @brief  Implementation of the xml file reader class.
 * 
 *  $Log: $
 */

#include "Objects/CaloHit.h"
#include "Objects/Track.h"

#include "Persistency/XmlFileReader.h"

namespace pandora
{

XmlFileReader::XmlFileReader(const pandora::Pandora &pandora, const std::string &fileName) :
    FileReader(pandora, fileName),
    m_pContainerXmlNode(NULL),
    m_pCurrentXmlElement(NULL),
    m_isAtFileStart(true)
{
    m_fileType = XML;
    m_pXmlDocument = new TiXmlDocument(fileName);

    if (!m_pXmlDocument->LoadFile())
    {
        std::cout << "XmlFileReader - Invalid xml file." << std::endl;
        delete m_pXmlDocument;
        throw StatusCodeException(STATUS_CODE_FAILURE);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

XmlFileReader::~XmlFileReader()
{
    delete m_pXmlDocument;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadHeader()
{
    m_pCurrentXmlElement = NULL;
    m_containerId = this->GetNextContainerId();

    if ((EVENT != m_containerId) && (GEOMETRY != m_containerId))
        return STATUS_CODE_FAILURE;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::GoToNextContainer()
{
    m_pCurrentXmlElement = NULL;

    if (m_isAtFileStart)
    {
        if (NULL == m_pContainerXmlNode)
            m_pContainerXmlNode = TiXmlHandle(m_pXmlDocument).FirstChildElement().Element();

        m_isAtFileStart = false;
    }
    else
    {
        if (NULL == m_pContainerXmlNode)
            throw StatusCodeException(STATUS_CODE_NOT_FOUND);

        m_pContainerXmlNode = m_pContainerXmlNode->NextSibling();
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

ContainerId XmlFileReader::GetNextContainerId()
{
    const std::string containerId((NULL != m_pContainerXmlNode) ? m_pContainerXmlNode->ValueStr() : "");

    if (std::string("Event") == containerId)
    {
        return EVENT;
    }
    else if (std::string("Geometry") == containerId)
    {
        return GEOMETRY;
    }
    else
    {
        return UNKNOWN_CONTAINER;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::GoToGeometry(const unsigned int geometryNumber)
{
    int nGeometriesRead(0);
    m_isAtFileStart = true;
    m_pContainerXmlNode = NULL;
    m_pCurrentXmlElement = NULL;

    if (GEOMETRY != this->GetNextContainerId())
        --nGeometriesRead;

    while (nGeometriesRead < static_cast<int>(geometryNumber))
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->GoToNextGeometry());
        ++nGeometriesRead;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::GoToEvent(const unsigned int eventNumber)
{
    int nEventsRead(0);
    m_isAtFileStart = true;
    m_pContainerXmlNode = NULL;
    m_pCurrentXmlElement = NULL;

    if (EVENT != this->GetNextContainerId())
        --nEventsRead;

    while (nEventsRead < static_cast<int>(eventNumber))
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->GoToNextEvent());
        ++nEventsRead;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadNextGeometryComponent()
{
    if (NULL == m_pCurrentXmlElement)
    {
        TiXmlHandle localHandle(m_pContainerXmlNode);
        m_pCurrentXmlElement = localHandle.FirstChild().Element();
    }
    else
    {
        m_pCurrentXmlElement = m_pCurrentXmlElement->NextSiblingElement();
    }

    if (NULL == m_pCurrentXmlElement)
    {
        this->GoToNextContainer();
        return STATUS_CODE_NOT_FOUND;
    }

    const std::string componentName(m_pCurrentXmlElement->ValueStr());

    if (std::string("SubDetector") == componentName)
    {
        return this->ReadSubDetector();
    }
    if (std::string("BoxGap") == componentName)
    {
        return this->ReadBoxGap();
    }
    else if (std::string("ConcentricGap") == componentName)
    {
        return this->ReadConcentricGap();
    }
    else
    {
        return STATUS_CODE_FAILURE;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadNextEventComponent()
{
    if (NULL == m_pCurrentXmlElement)
    {
        TiXmlHandle localHandle(m_pContainerXmlNode);
        m_pCurrentXmlElement = localHandle.FirstChild().Element();
    }
    else
    {
        m_pCurrentXmlElement = m_pCurrentXmlElement->NextSiblingElement();
    }

    if (NULL == m_pCurrentXmlElement)
    {
        this->GoToNextContainer();
        return STATUS_CODE_NOT_FOUND;
    }

    const std::string componentName(m_pCurrentXmlElement->ValueStr());

    if (std::string("CaloHit") == componentName)
    {
        return this->ReadCaloHit();
    }
    else if (std::string("Track") == componentName)
    {
        return this->ReadTrack();
    }
    else if (std::string("MCParticle") == componentName)
    {
        return this->ReadMCParticle();
    }
    else if (std::string("Relationship") == componentName)
    {
        return this->ReadRelationship();
    }
    else
    {
        return STATUS_CODE_FAILURE;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadSubDetector()
{
    if (GEOMETRY != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::Geometry::SubDetector::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pSubDetectorFactory->Read(parameters, *this));

    std::string subDetectorName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("SubDetectorName", subDetectorName));
    unsigned int subDetectorTypeInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("SubDetectorType", subDetectorTypeInput));
    const SubDetectorType subDetectorType(static_cast<SubDetectorType>(subDetectorTypeInput));
    float innerRCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerRCoordinate", innerRCoordinate));
    float innerZCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerZCoordinate", innerZCoordinate));
    float innerPhiCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerPhiCoordinate", innerPhiCoordinate));
    unsigned int innerSymmetryOrder(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerSymmetryOrder", innerSymmetryOrder));
    float outerRCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterRCoordinate", outerRCoordinate));
    float outerZCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterZCoordinate", outerZCoordinate));
    float outerPhiCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterPhiCoordinate", outerPhiCoordinate));
    unsigned int outerSymmetryOrder(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterSymmetryOrder", outerSymmetryOrder));
    bool isMirroredInZ(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("IsMirroredInZ", isMirroredInZ));
    unsigned int nLayers(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("NLayers", nLayers));

    parameters.m_subDetectorName = subDetectorName;
    parameters.m_subDetectorType = subDetectorType;
    parameters.m_innerRCoordinate = innerRCoordinate;
    parameters.m_innerZCoordinate = innerZCoordinate;
    parameters.m_innerPhiCoordinate = innerPhiCoordinate;
    parameters.m_innerSymmetryOrder = innerSymmetryOrder;
    parameters.m_outerRCoordinate = outerRCoordinate;
    parameters.m_outerZCoordinate = outerZCoordinate;
    parameters.m_outerPhiCoordinate = outerPhiCoordinate;
    parameters.m_outerSymmetryOrder = outerSymmetryOrder;
    parameters.m_isMirroredInZ = isMirroredInZ;
    parameters.m_nLayers = nLayers;

    if (nLayers > 0)
    {
        FloatVector closestDistanceToIp, nRadiationLengths, nInteractionLengths;
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ClosestDistanceToIp", closestDistanceToIp));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("NRadiationLengths", nRadiationLengths));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("NInteractionLengths", nInteractionLengths));

        if ((closestDistanceToIp.size() != nLayers) || (nRadiationLengths.size() != nLayers) || (nInteractionLengths.size() != nLayers))
            return STATUS_CODE_FAILURE;

        for (unsigned int iLayer = 0; iLayer < nLayers; ++iLayer)
        {
            PandoraApi::Geometry::LayerParameters layerParameters;
            layerParameters.m_closestDistanceToIp = closestDistanceToIp[iLayer];
            layerParameters.m_nRadiationLengths = nRadiationLengths[iLayer];
            layerParameters.m_nInteractionLengths = nInteractionLengths[iLayer];
            parameters.m_layerParametersList.push_back(layerParameters);
        }
    }

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::Geometry::SubDetector::Create(*m_pPandora, parameters, *m_pSubDetectorFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadBoxGap()
{
    if (GEOMETRY != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::Geometry::BoxGap::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pBoxGapFactory->Read(parameters, *this));

    CartesianVector vertex(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Vertex", vertex));
    CartesianVector side1(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Side1", side1));
    CartesianVector side2(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Side2", side2));
    CartesianVector side3(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Side3", side3));

    parameters.m_vertex = vertex;
    parameters.m_side1 = side1;
    parameters.m_side2 = side2;
    parameters.m_side3 = side3;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::Geometry::BoxGap::Create(*m_pPandora, parameters, *m_pBoxGapFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadConcentricGap()
{
    if (GEOMETRY != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::Geometry::ConcentricGap::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pConcentricGapFactory->Read(parameters, *this));

    float minZCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("MinZCoordinate", minZCoordinate));
    float maxZCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("MaxZCoordinate", maxZCoordinate));
    float innerRCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerRCoordinate", innerRCoordinate));
    float innerPhiCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerPhiCoordinate", innerPhiCoordinate));
    unsigned int innerSymmetryOrder(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InnerSymmetryOrder", innerSymmetryOrder));
    float outerRCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterRCoordinate", outerRCoordinate));
    float outerPhiCoordinate(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterPhiCoordinate", outerPhiCoordinate));
    unsigned int outerSymmetryOrder(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("OuterSymmetryOrder", outerSymmetryOrder));

    parameters.m_minZCoordinate = minZCoordinate;
    parameters.m_maxZCoordinate = maxZCoordinate;
    parameters.m_innerRCoordinate = innerRCoordinate;
    parameters.m_innerPhiCoordinate = innerPhiCoordinate;
    parameters.m_innerSymmetryOrder = innerSymmetryOrder;
    parameters.m_outerRCoordinate = outerRCoordinate;
    parameters.m_outerPhiCoordinate = outerPhiCoordinate;
    parameters.m_outerSymmetryOrder = outerSymmetryOrder;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::Geometry::ConcentricGap::Create(*m_pPandora, parameters, *m_pConcentricGapFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadCaloHit()
{
    if (EVENT != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::CaloHit::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pCaloHitFactory->Read(parameters, *this));

    unsigned int cellGeometryInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CellGeometry", cellGeometryInput));
    const CellGeometry cellGeometry(static_cast<CellGeometry>(cellGeometryInput));
    CartesianVector positionVector(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("PositionVector", positionVector));
    CartesianVector expectedDirection(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ExpectedDirection", expectedDirection));
    CartesianVector cellNormalVector(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CellNormalVector", cellNormalVector));
    float cellThickness(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CellThickness", cellThickness));
    float nCellRadiationLengths(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("NCellRadiationLengths", nCellRadiationLengths));
    float nCellInteractionLengths(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("NCellInteractionLengths", nCellInteractionLengths));
    float time(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Time", time));
    float inputEnergy(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("InputEnergy", inputEnergy));
    float mipEquivalentEnergy(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("MipEquivalentEnergy", mipEquivalentEnergy));
    float electromagneticEnergy(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ElectromagneticEnergy", electromagneticEnergy));
    float hadronicEnergy(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("HadronicEnergy", hadronicEnergy));
    bool isDigital(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("IsDigital", isDigital));
    unsigned int hitTypeInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("HitType", hitTypeInput));
    const HitType hitType(static_cast<HitType>(hitTypeInput));
    unsigned int hitRegionInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("HitRegion", hitRegionInput));
    const HitRegion hitRegion(static_cast<HitRegion>(hitRegionInput));
    unsigned int layer(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Layer", layer));
    bool isInOuterSamplingLayer(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("IsInOuterSamplingLayer", isInOuterSamplingLayer));
    const void *pParentAddress(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ParentCaloHitAddress", pParentAddress));
    float cellSize0(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CellSize0", cellSize0));
    float cellSize1(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CellSize1", cellSize1));

    parameters.m_positionVector = positionVector;
    parameters.m_expectedDirection = expectedDirection;
    parameters.m_cellNormalVector = cellNormalVector;
    parameters.m_cellGeometry = cellGeometry;
    parameters.m_cellSize0 = cellSize0;
    parameters.m_cellSize1 = cellSize1;
    parameters.m_cellThickness = cellThickness;
    parameters.m_nCellRadiationLengths = nCellRadiationLengths;
    parameters.m_nCellInteractionLengths = nCellInteractionLengths;
    parameters.m_time = time;
    parameters.m_inputEnergy = inputEnergy;
    parameters.m_mipEquivalentEnergy = mipEquivalentEnergy;
    parameters.m_electromagneticEnergy = electromagneticEnergy;
    parameters.m_hadronicEnergy = hadronicEnergy;
    parameters.m_isDigital = isDigital;
    parameters.m_hitType = hitType;
    parameters.m_hitRegion = hitRegion;
    parameters.m_layer = layer;
    parameters.m_isInOuterSamplingLayer = isInOuterSamplingLayer;
    parameters.m_pParentAddress = pParentAddress;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, parameters, *m_pCaloHitFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadTrack()
{
    if (EVENT != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::Track::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pTrackFactory->Read(parameters, *this));

    float d0(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("D0", d0));
    float z0(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Z0", z0));
    int particleId(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ParticleId", particleId));
    int charge(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Charge", charge));
    float mass(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Mass", mass));
    CartesianVector momentumAtDca(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("MomentumAtDca", momentumAtDca));
    TrackState trackStateAtStart(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("TrackStateAtStart", trackStateAtStart));
    TrackState trackStateAtEnd(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("TrackStateAtEnd", trackStateAtEnd));
    TrackState trackStateAtCalorimeter(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("TrackStateAtCalorimeter", trackStateAtCalorimeter));
    float timeAtCalorimeter(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("TimeAtCalorimeter", timeAtCalorimeter));
    bool reachesCalorimeter(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ReachesCalorimeter", reachesCalorimeter));
    bool isProjectedToEndCap(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("IsProjectedToEndCap", isProjectedToEndCap));
    bool canFormPfo(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CanFormPfo", canFormPfo));
    bool canFormClusterlessPfo(false);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("CanFormClusterlessPfo", canFormClusterlessPfo));
    const void *pParentAddress(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ParentTrackAddress", pParentAddress));

    parameters.m_d0 = d0;
    parameters.m_z0 = z0;
    parameters.m_particleId = particleId;
    parameters.m_charge = charge;
    parameters.m_mass = mass;
    parameters.m_momentumAtDca = momentumAtDca;
    parameters.m_trackStateAtStart = trackStateAtStart;
    parameters.m_trackStateAtEnd = trackStateAtEnd;
    parameters.m_trackStateAtCalorimeter = trackStateAtCalorimeter;
    parameters.m_timeAtCalorimeter = timeAtCalorimeter;
    parameters.m_reachesCalorimeter = reachesCalorimeter;
    parameters.m_isProjectedToEndCap = isProjectedToEndCap;
    parameters.m_canFormPfo = canFormPfo;
    parameters.m_canFormClusterlessPfo = canFormClusterlessPfo;
    parameters.m_pParentAddress = pParentAddress;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::Track::Create(*m_pPandora, parameters, *m_pTrackFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadMCParticle()
{
    if (EVENT != m_containerId)
        return STATUS_CODE_FAILURE;

    PandoraApi::MCParticle::Parameters parameters;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, m_pMCParticleFactory->Read(parameters, *this));

    float energy(0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Energy", energy));
    CartesianVector momentum(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Momentum", momentum));
    CartesianVector vertex(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Vertex", vertex));
    CartesianVector endpoint(0.f, 0.f, 0.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Endpoint", endpoint));
    int particleId(-std::numeric_limits<int>::max());
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("ParticleId", particleId));
    unsigned int mcParticleTypeInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("MCParticleType", mcParticleTypeInput));
    const MCParticleType mcParticleType(static_cast<MCParticleType>(mcParticleTypeInput));
    const void *pParentAddress(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Uid", pParentAddress));

    parameters.m_energy = energy;
    parameters.m_momentum = momentum;
    parameters.m_vertex = vertex;
    parameters.m_endpoint = endpoint;
    parameters.m_particleId = particleId;
    parameters.m_mcParticleType = mcParticleType;
    parameters.m_pParentAddress = pParentAddress;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, parameters, *m_pMCParticleFactory));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode XmlFileReader::ReadRelationship()
{
    if (EVENT != m_containerId)
        return STATUS_CODE_FAILURE;

    unsigned int relationshipIdInput(0);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("RelationshipId", relationshipIdInput));
    const RelationshipId relationshipId(static_cast<RelationshipId>(relationshipIdInput));
    const void *address1(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Address1", address1));
    const void *address2(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Address2", address2));
    float weight(1.f);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->ReadVariable("Weight", weight));

    switch (relationshipId)
    {
    case CALO_HIT_TO_MC:
        return PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, address1, address2, weight);
    case TRACK_TO_MC:
        return PandoraApi::SetTrackToMCParticleRelationship(*m_pPandora, address1, address2, weight);
    case MC_PARENT_DAUGHTER:
        return PandoraApi::SetMCParentDaughterRelationship(*m_pPandora, address1, address2);
    case TRACK_PARENT_DAUGHTER:
        return PandoraApi::SetTrackParentDaughterRelationship(*m_pPandora, address1, address2);
    case TRACK_SIBLING:
        return PandoraApi::SetTrackSiblingRelationship(*m_pPandora, address1, address2);
    default:
        return STATUS_CODE_FAILURE;
    }

    return STATUS_CODE_SUCCESS;
}

} // namespace pandora
