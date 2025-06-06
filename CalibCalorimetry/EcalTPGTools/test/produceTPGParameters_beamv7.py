import FWCore.ParameterSet.Config as cms
import CondTools.Ecal.db_credentials as auth
import FWCore.ParameterSet.VarParsing as VarParsing

process = cms.Process("ProdTPGParam")
options = VarParsing.VarParsing('tpg')

options.register ('writeToDB',
                  0, # default value
                  VarParsing.VarParsing.multiplicity.singleton, 
                  VarParsing.VarParsing.varType.int,         
                  "Write conditions to DB")
options.register ('outFile',
                  'TPG_beamv7_trans_rrrrrr_spikekill_2021.txt', 
                  VarParsing.VarParsing.multiplicity.singleton, 
                  VarParsing.VarParsing.varType.string,         
                  "Output file")
options.register ('transparency',
                  'TRANSPARENCY_2018/hourly_rrrrrr', 
                  VarParsing.VarParsing.multiplicity.singleton, 
                  VarParsing.VarParsing.varType.string,         
                  "Transparency conditions file")

options.parseArguments()

# Calo geometry service model
process.load("Configuration.StandardSequences.GeometryDB_cff")

# ecal mapping
process.eegeom = cms.ESSource("EmptyESSource",
    recordName = cms.string('EcalMappingRcd'),
    iovIsRunNotTime = cms.bool(True),
    firstValid = cms.vuint32(1)
)

# Get hardcoded conditions the same used for standard digitization before CMSSW_3_1_x
## process.load("CalibCalorimetry.EcalTrivialCondModules.EcalTrivialCondRetriever_cfi")
# or Get DB parameters
# process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')
process.load("CondCore.CondDB.CondDB_cfi")

process.CondDB.connect = 'frontier://FrontierProd/CMS_CONDITIONS'
process.CondDB.DBParameters.authenticationPath = '/nfshome0/popcondev/conddb' ###P5 stuff


process.PoolDBESSource = cms.ESSource("PoolDBESSource",
                                          process.CondDB,
                                          toGet = cms.VPSet(
              cms.PSet(
            record = cms.string('EcalPedestalsRcd'),
                    #tag = cms.string('EcalPedestals_v5_online')
                    #tag = cms.string('EcalPedestals_2009runs_hlt') ### obviously diff w.r.t previous
                    tag = cms.string('EcalPedestals_hlt'), ### modif-alex 22/02/2011
                 ),
              cms.PSet(
            record = cms.string('EcalADCToGeVConstantRcd'),
                    #tag = cms.string('EcalADCToGeVConstant_EBg50_EEnoB_new')
                    #tag = cms.string('EcalADCToGeVConstant_2009runs_express') ### the 2 ADCtoGEV in EB and EE are diff w.r.t previous
                    tag = cms.string('EcalADCToGeVConstant_V1_hlt')
                 ),
              cms.PSet(
            record = cms.string('EcalIntercalibConstantsRcd'),
                    #tag = cms.string('EcalIntercalibConstants_EBg50_EEnoB_new')
                    #tag = cms.string('EcalIntercalibConstants_2009runs_express') ### differs from previous
                    tag = cms.string('EcalIntercalibConstants_V1_hlt')
                 ),
              cms.PSet(
            record = cms.string('EcalGainRatiosRcd'),
                    #tag = cms.string('EcalGainRatios_TestPulse_online')
                    tag = cms.string('EcalGainRatios_TestPulse_express') ### no diff w.r.t previous
                 ),
              cms.PSet(
                record = cms.string('EcalMappingElectronicsRcd'),
                    tag = cms.string('EcalMappingElectronics_EEMap_v1_mc')
                 ),
              cms.PSet(
                record = cms.string('EcalLaserAlphasRcd'),
                tag = cms.string('EcalLaserAlphas_EB_sic1_btcp152_EE_sic1_btcp116')
              )
               )
             )


#########################
process.source = cms.Source("EmptySource",
       ##firstRun = cms.untracked.uint32(100000000) ### need to use latest run to pick-up update values from DB
       firstRun = cms.untracked.uint32(161310)
)


process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)

db_reader_account = 'CMS_ECAL_CONF'
db_service,db_user,db_pwd = auth.get_db_credentials( db_reader_account )


##process.TPGParamProducer = cms.EDFilter("EcalTPGParamBuilder",
process.TPGParamProducer = cms.EDAnalyzer("EcalTPGParamBuilder",

    #### inputs/ouputs control ####
    writeToDB  = cms.bool(options.writeToDB),
    allowDBEE  = cms.bool(True),

#    DBsid   = cms.string('cms_omds_lb'), ## real DB on P5
    DBuser = cms.string(db_user),
    DBpass = cms.string(db_pwd),
    DBsid  = cms.string(db_service),
    DBport  = cms.uint32(10121),

    TPGWritePed = cms.uint32(1), # can be 1=load ped from offline DB  0=use previous ped NN=use ped from ped_conf_id=NN
    TPGWriteLin = cms.uint32(1),
    TPGWriteSli = cms.uint32(1),
    TPGWriteWei = cms.uint32(1),
    TPGWriteWei2 = cms.uint32(1), # odd weights
    TPGWriteLut = cms.uint32(1),
    TPGWriteFgr = cms.uint32(1),
    TPGWriteSpi = cms.uint32(1),
    TPGWriteDel = cms.uint32(1),
    TPGWriteBxt = cms.uint32(0), # these can be 0=use same as existing number for this tag or NN=use badxt from bxt_conf_id=NN
    TPGWriteBtt = cms.uint32(0),
    TPGWriteBst = cms.uint32(0),

    writeToFiles = cms.bool(True),
    outFile = cms.string(options.outFile),

   #### TPG config tag and version (if not given it will be automatically given ) ####
    TPGtag = cms.string('BEAMV7_TEST'),
    TPGversion = cms.uint32(1),

   #### TPG calculation parameters ####
    useTransverseEnergy = cms.bool(True),    ## true when TPG computes transverse energy, false for energy
    Et_sat_EB = cms.double(128.0),            ## Saturation value (in GeV) of the TPG before the compressed-LUT (rem: with 35.84 the TPG_LSB = crystal_LSB)
    Et_sat_EE = cms.double(128.0),            ## Saturation value (in GeV) of the TPG before the compressed-LUT (rem: with 35.84 the TPG_LSB = crystal_LSB)

    sliding = cms.uint32(0),                 ## Parameter used for the FE data format, should'nt be changed

    weight_timeShift = cms.double(0.),       ## weights are computed shifting the timing of the shape by this amount in ns: val>0 => shape shifted to the right
    weight_sampleMax = cms.uint32(3),        ## position of the maximum among the 5 samples used by the TPG amplitude filter
    weight_unbias_recovery = cms.bool(True), ## true if weights after int conversion are forced to have sum=0. Pb, in that case it can't have sum f*w = 1

    weight_even_computeFromShape = cms.bool(True),   ## If False do not compute the even weights but read them from files
    weight_even_idMapFile = cms.string('EcalTPGIdMap_2018_PU50_S2.txt'),
    weight_even_weightGroupFile = cms.string('EcalTPGWeightGroup_2018_PU50_S2.txt'),
    weight_odd_idMapFile = cms.string('EcalTPGIdMap_odd.txt'),
    weight_odd_weightGroupFile = cms.string('EcalTPGWeightGroup_odd.txt'),

    weight_useDoubleWeights =  cms.bool(False),  # If True the double weights configuration is created, if False the wei2_id is set to 1, the Run2 default (special meaning for the DAQ)
    TPmode_EnableEBOddFilter = cms.bool(False), # TPmode configs are used only is weight_useDoubleWeights is True
    TPmode_EnableEEOddFilter = cms.bool(False),
    TPmode_EnableEBOddPeakFinder = cms.bool(False),
    TPmode_EnableEEOddPeakFinder = cms.bool(False),
    TPmode_DisableEBEvenPeakFinder = cms.bool(False),
    TPmode_DisableEEEvenPeakFinder = cms.bool(False),
    TPmode_FenixEBStripOutput      = cms.uint32(0),
    TPmode_FenixEEStripOutput      = cms.uint32(0),
    TPmode_FenixEBStripInfobit2    = cms.uint32(0),
    TPmode_FenixEEStripInfobit2    = cms.uint32(0),
    TPmode_FenixEBTcpOutput        = cms.uint32(0),
    TPmode_FenixEBTcpInfobit1      = cms.uint32(0),
    TPmode_FenixEETcpOutput        = cms.uint32(0),
    TPmode_FenixEETcpInfobit1      = cms.uint32(0),

    forcedPedestalValue = cms.int32(-3),     ## use this value instead of getting it from DB or MC
                                             ## -1: means use value from DB or MC.
                                              ## -2: ped12 = 0 used to cope with FENIX bug
                                             ## -3: used with sFGVB: baseline subtracted is pedestal-offset*sin(theta)/G with G=mult*2^-(shift+2)
    pedestal_offset =  cms.uint32(150),      ## pedestal offset used with option forcedPedestalValue = -3

    useInterCalibration = cms.bool(True),    ## use or not values from DB. If not, 1 is assumed

    timing_delays_EB = cms.string('Delays_EB.txt'), # timing delays for latency EB / TT
    timing_delays_EE = cms.string('Delays_EE.txt'), # timing delays for latency EE / strip
    timing_phases_EB = cms.string('Phases_EB.txt'), # TCC phase setting for EB / TT
    timing_phases_EE = cms.string('Phases_EE.txt'), # TCC phase setting for EE / strip

    useTransparencyCorr = cms.bool(True),   ## true if you want to correct TPG for transparency change in EE
    transparency_corrections = cms.string(options.transparency), # transparency corr to be used to compute linearizer parameters 1/crystal

    SFGVB_Threshold = cms.uint32(16),             ## (adc) SFGVB threshold in FE
    SFGVB_lut = cms.uint32(0xfffefee8),           ## SFGVB LUT in FE
    SFGVB_SpikeKillingThreshold = cms.int32(16),  ## (GeV) Spike killing threshold applied in TPG ET in TCC (-1 no killing)

    forceEtaSlice = cms.bool(False),         ## when true, same linearization coeff for all crystals belonging to a given eta slice (tower)

    LUT_option = cms.string('Linear'),       ## compressed LUT option can be: "Identity", "Linear", "EcalResolution"
    LUT_threshold_EB = cms.double(0.50),    ## All Trigger Primitives <= threshold (in GeV) will be set to 0
    LUT_threshold_EE = cms.double(0.50),    ## All Trigger Primitives <= threshold (in GeV) will be set to 0
    LUT_stochastic_EB = cms.double(0.03),    ## Stochastic term of the ECAL-EB ET resolution (used only if LUT_option="EcalResolution")
    LUT_noise_EB = cms.double(0.2),          ## noise term (GeV) of the ECAL-EB ET resolution (used only if LUT_option="EcalResolution")
    LUT_constant_EB = cms.double(0.005),     ## constant term of the ECAL-EB ET resolution (used only if LUT_option="EcalResolution")
    LUT_stochastic_EE = cms.double(0.03),    ## Stochastic term of the ECAL-EE ET resolution (used only if LUT_option="EcalResolution")
    LUT_noise_EE = cms.double(0.2),          ## noise term (GeV) of the ECAL-EE ET resolution (used only if LUT_option="EcalResolution")
    LUT_constant_EE = cms.double(0.005),     ## constant term of the ECAL-EE ET resolution (used only if LUT_option="EcalResolution")

    TTF_lowThreshold_EB = cms.double(2),   ## EB Trigger Tower Flag low threshold in GeV
    TTF_highThreshold_EB = cms.double(4),  ## EB Trigger Tower Flag high threshold in GeV
    TTF_lowThreshold_EE = cms.double(2),   ## EE Trigger Tower Flag low threshold in GeV
    TTF_highThreshold_EE = cms.double(4),  ## EE Trigger Tower Flag high threshold in GeV

    FG_lowThreshold_EB = cms.double(3.9),      ## EB Fine Grain Et low threshold in GeV
    FG_highThreshold_EB = cms.double(3.9),     ## EB Fine Grain Et high threshold in GeV
    FG_lowRatio_EB = cms.double(0.9),          ## EB Fine Grain low-ratio
    FG_highRatio_EB = cms.double(0.9),         ## EB Fine Grain high-ratio
    FG_lut_EB = cms.uint32(0x08),              ## EB Fine Grain Look-up table. Put something != 0 if you really know what you do!
    FG_Threshold_EE = cms.double(18.75),       ## EE Fine threshold in GeV
    FG_lut_strip_EE = cms.uint32(0xfffefee8),  ## EE Fine Grain strip Look-up table
    FG_lut_tower_EE = cms.uint32(0)            ## EE Fine Grain tower Look-up table
)

process.p = cms.Path(process.TPGParamProducer)
