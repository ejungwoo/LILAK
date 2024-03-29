#include "TEveViewer.h"
#include "TGLViewer.h"
#include "TEveGeoNode.h"
#include "TEveManager.h"
#include "TEveScene.h"
#include "TEveWindow.h"
#include "TEveWindowManager.h"
#include "TEveGedEditor.h"
#include "TEveBrowser.h"
#include "TRootBrowser.h"
#include "TBrowser.h"
#include "TGTab.h"
#include "TVirtualX.h"
#include "TGWindow.h"
#include "TGeoManager.h"
#include "TRootEmbeddedCanvas.h"
#include "TEvePointSet.h"
#include "TEveLine.h"
#include "TEveArrow.h"

#include "TStyle.h"

#include "LKLogger.hpp"
#include "LKRun.hpp"
#include "LKEveTask.hpp"

ClassImp(LKEveTask)

LKEveTask::LKEveTask()
    :LKTask("LKEveTask","")
{
}

bool LKEveTask::Init()
{
    auto run = LKRun::GetRun();

    fNumBranches = run -> GetNumBranches();

    fSelTrkIDs    = fPar -> GetParVInt("eveSelectTrackIDs");
    fIgnTrkIDs    = fPar -> GetParVInt("eveIgnoreTrackIDs");
    fSelPntIDs    = fPar -> GetParVInt("eveSelectTrackParentIDs");
    fIgnPntIDs    = fPar -> GetParVInt("eveIgnoreTrackParentIDs");
    fSelPDGs      = fPar -> GetParVInt("eveSelectTrackPDGs");
    fIgnPDGs      = fPar -> GetParVInt("eveIgnoreTrackPDGs");
    fSelMCIDs     = fPar -> GetParVInt("eveSelectMCIDs");
    fIgnMCIDs     = fPar -> GetParVInt("eveIgnoreMCIDs");
    fSelHitPntIDs = fPar -> GetParVInt("eveSelectHitParentIDs");
    fIgnHitPntIDs = fPar -> GetParVInt("eveIgnoreHitParentIDs");
    fSelBranchNames = fPar -> GetParVString("eveSelectBranches");

    fNumSelectedBranches = fSelBranchNames.size();
    if (fNumSelectedBranches==0) {
        fNumSelectedBranches = fNumBranches;
        for (auto iBranch=0; iBranch<fNumSelectedBranches; ++iBranch)
            fSelBranchNames.push_back(run->GetBranchName(iBranch));
    }
    else {
        vector<TString> tempBranchNames;
        for (auto iBranch=0; iBranch<fNumSelectedBranches; ++iBranch) {
            TString branchName = fSelBranchNames.at(iBranch);
            auto branchPtr = run -> GetBranchA(branchName);
            if (branchPtr == nullptr) {
                lk_error << "No eve-branch name " << branchName << endl;
                continue;
            }
            tempBranchNames.push_back(branchName);
        }
        fSelBranchNames.clear();
        for (auto branchName : tempBranchNames)
            fSelBranchNames.push_back(branchName);
    }

    ConfigureDisplayWindow();

    return true;
}

void LKEveTask::Exec(Option_t*)
{
    RunEve();
}

void LKEveTask::DrawEve3D()
{
    auto run = LKRun::GetRun();

    if (gEve!=nullptr) {
        auto numEveEvents = fEveEventManagerArray -> GetEntries();
        for (auto iEveEvent=0; iEveEvent<numEveEvents; ++iEveEvent) {
            ((TEveEventManager *) fEveEventManagerArray -> At(iEveEvent)) -> RemoveElements();
        }
    }

    if (gEve == nullptr)
        ConfigureDisplayWindow();

    bool removePointTrack = (fPar->CheckPar("eveRemovePointTrack")) ? (fPar->GetParBool("eveRemovePointTrack")) : false;

    for (Int_t iBranch = 0; iBranch < fNumSelectedBranches; ++iBranch)
    {
        TString branchName = fSelBranchNames.at(iBranch);
        auto branch = run -> GetBranchA(branchName);
        if (branch == nullptr) {
            lk_error << "No eve-branch name " << branchName << endl;
            continue;
        }
        if (branch -> GetEntries() == 0)
            continue;

        auto objSample = branch -> At(0);
        if (objSample -> InheritsFrom("LKContainer") == false)
            continue;

        bool isTracklet = (objSample -> InheritsFrom("LKTracklet")) ? true : false;
        bool isHit = (objSample -> InheritsFrom("LKHit")) ? true : false;

        LKContainer *eveObj = (LKContainer *) objSample;
        if (fSelBranchNames.size() == 0 || !eveObj -> DrawByDefault())
            continue;

        auto eveEvent = (TEveEventManager *) fEveEventManagerArray -> FindObject(branchName);
        if (eveEvent==nullptr) {
            eveEvent = new TEveEventManager(branchName);
            fEveEventManagerArray -> Add(eveEvent);
            gEve -> AddEvent(eveEvent);
        }

        int numSelected = 0;

        Int_t nObjects = branch -> GetEntries();
        if (isTracklet)
        {
            for (Int_t iObject = 0; iObject < nObjects; ++iObject) {
                LKTracklet *tracklet = (LKTracklet *) branch -> At(iObject);

                /*
                   if (removePointTrack)
                   if (tracklet -> InheritsFrom("LKMCTrack"))
                   if (((LKMCTrack *) tracklet) -> GetNumVertices() < 2)
                   continue;
                 */

                if (!SelectTrack(tracklet))
                    continue;

                auto eveLine = (TEveLine *) tracklet -> CreateEveElement();
                tracklet -> SetEveElement(eveLine, fEveScale);
                SetEveLineAtt(eveLine,branchName);
                eveEvent -> AddElement(eveLine);
                numSelected++;
            }
        }
        else if (eveObj -> IsEveSet())
        {
            auto eveSet = (TEvePointSet *) eveObj -> CreateEveElement();
            TString name = Form("%s_%s",eveSet->GetElementName(),branchName.Data());
            eveSet -> SetElementName(name);
            for (Int_t iObject = 0; iObject < nObjects; ++iObject) {
                eveObj = (LKContainer *) branch -> At(iObject);

                if (isHit)  {
                    LKHit *hit = (LKHit *) branch -> At(iObject);
                    hit -> SetSortValue(1);

                    if (!SelectHit(hit))
                        continue;
                }

                eveObj -> AddToEveSet(eveSet, fEveScale);
                numSelected++;
            }
            SetEveMarkerAtt(eveSet, branchName);
            eveEvent -> AddElement(eveSet);
        }
        else {
            for (Int_t iObject = 0; iObject < nObjects; ++iObject) {
                eveObj = (LKContainer *) branch -> At(iObject);
                auto eveElement = eveObj -> CreateEveElement();
                eveObj -> SetEveElement(eveElement, fEveScale);
                TString name = Form("%s_%d",eveElement -> GetElementName(),iObject);
                eveElement -> SetElementName(name);
                eveEvent -> AddElement(eveElement);
                numSelected++;
            }
        }

        lk_info << "Drawing " << branchName << " [" << branch -> At(0) -> ClassName() << "] " << numSelected << "(" << branch -> GetEntries() << ")" << endl;
    }

    if (fPar->CheckPar("eveAxisOrigin")) {
        Double_t length = 100.;
        if (fPar->CheckPar("eveAxisLength"))
            length = fPar->GetParDouble("eveAxisLength");
        TVector3 origin = fPar -> GetParV3("eveAxisOrigin");
        for (auto iaxis : {0,1,2}) {
            TVector3 direction;
            if (iaxis==0) direction = TVector3(length,0,0);
            else if (iaxis==1) direction = TVector3(0,length,0);
            else if (iaxis==2) direction = TVector3(0,0,length);
            auto axis = new TEveArrow(direction.X(),direction.Y(),direction.Z(),origin.X(),origin.Y(),origin.Z()); // TODO
            axis -> SetElementName(TString("axis_")+iaxis);
            if (iaxis==0) axis -> SetMainColor(kRed);
            else if (iaxis==1) axis -> SetMainColor(kBlue);
            else if (iaxis==2) axis -> SetMainColor(kBlack);
            ((TEveEventManager *) fEveEventManagerArray -> At(0)) -> AddElement(axis);
        }
    }

    gEve -> Redraw3D();
}

void LKEveTask::DrawDetectorPlanes()
{
    auto run = LKRun::GetRun();

    ConfigureDetectorPlanes();

    if (fGraphChannelBoundaryNb[0] == nullptr) { // TODO
        for (Int_t iGraph = 0; iGraph < 20; ++iGraph) {
            fGraphChannelBoundaryNb[iGraph] = new TGraph();
            fGraphChannelBoundaryNb[iGraph] -> SetLineColor(kGreen);
            fGraphChannelBoundaryNb[iGraph] -> SetLineWidth(2);
        }
    }

    auto hitArray = run -> GetBranchA("Hit");
    auto padArray = run -> GetBranchA("Pad");

    auto ppHistMin = 0.01;
    if (fPar->CheckPar("evePPHistMin"))
        ppHistMin = fPar -> GetParDouble("evePPHistMin");

    auto numPlanes = fDetectorSystem -> GetNumPlanes();
    for (auto iPlane = 0; iPlane < numPlanes; ++iPlane)
    {
        auto plane = fDetectorSystem -> GetDetectorPlane(iPlane);
        lk_info << "Drawing " << plane -> GetName() << endl;

        auto histPlane = plane -> GetHist();
        histPlane -> SetMinimum(ppHistMin);
        histPlane -> Reset();

        auto cvs = (TCanvas *) fCvsDetectorPlaneArray -> At(iPlane);

        /*
        if (plane -> InheritsFrom("LKPadPlane"))
        {
            auto padplane = (LKPadPlane *) plane;

            bool exist_hit = false;
            bool exist_pad = false;

            if (hitArray != nullptr)
                exist_hit = true;
            else {
                for (Int_t iBranch = 0; iBranch < fNumSelectedBranches; ++iBranch)
                {
                    TString branchName = fSelBranchNames.at(iBranch);
                    if (branchName.Index("Hit")==0) {
                        lk_info << branchName << " is to be filled to pad plane" << endl;
                        hitArray = (TClonesArray *) fBranchPtrMap[branchName];
                        hitArray -> Print();
                        exist_hit = true;
                        break;
                    }
                }
            }
            if (padArray != nullptr)
                exist_pad = true;

            if (!exist_hit && !exist_pad) {
                cvs -> cd();
                histPlane -> Draw();
                plane -> DrawFrame();
                continue;
            }

            padplane -> Clear();
            if (exist_hit) padplane -> SetHitArray(hitArray);
            if (exist_pad) padplane -> SetPadArray(padArray);

            if (fPar -> CheckPar("evePPFillOption"))
            {
                auto fillOption = fPar -> GetParString("evePPFillOption");
                lk_info << "Filling " << fillOption << " to PadPlane" << endl;
                padplane -> FillDataToHist(fillOption);
            }
            else if (exist_hit)
            {
                lk_info << "Filling Hits to PadPlane" << endl;
                padplane -> FillDataToHist("hit");
            }
            else if (exist_pad)
            {
                lk_info << "Filling Pads to PadPlane" << endl;
                padplane -> FillDataToHist("out");
            }
        }
        */

        cvs -> Clear();
        cvs -> cd();
        histPlane -> DrawClone("colz");
        histPlane -> Reset();
        histPlane -> Draw("same");
        plane -> DrawFrame();

        auto axis1 = plane -> GetAxis1();
        auto axis2 = plane -> GetAxis2();

        for (Int_t iBranch = 0; iBranch < fNumSelectedBranches; ++iBranch)
        {
            TClonesArray *branch = nullptr;
            if (fNumSelectedBranches != 0) {
                TString branchName = fSelBranchNames.at(iBranch);
                branch = run -> GetBranchA(branchName);
            }
            else
                branch = run -> GetBranchA(iBranch);

            TObject *objSample = nullptr;

            Int_t numTracklets = branch -> GetEntries();
            if (numTracklets != 0) {
                objSample = branch -> At(0);
                if (objSample -> InheritsFrom("LKContainer") == false || objSample -> InheritsFrom("LKTracklet") == false)
                    continue;
            }
            else
                continue;

            auto trackletSample = (LKTracklet *) objSample;
            if (trackletSample -> DoDrawOnDetectorPlane())
            {
                for (auto iTracklet = 0; iTracklet < numTracklets; ++iTracklet) {
                    auto tracklet = (LKTracklet *) branch -> At(iTracklet);
                    if (!SelectTrack(tracklet))
                        continue;

                    tracklet -> TrajectoryOnPlane(axis1,axis2) -> Draw("samel"); // @todo
                }
            }
        }
    }

    // @todo palette is changed when drawing top node because of TGeoMan(?)
    gStyle -> SetPalette(kBird);
}

void LKEveTask::RunEve()
{
    Bool_t drawEve3D = true;
    Bool_t drawDetectorPlanes = true;

    if (drawEve3D) DrawEve3D();
    if (drawDetectorPlanes) DrawDetectorPlanes();
}

void LKEveTask::ConfigureDisplayWindow()
{
    if (fDetectorSystem -> GetEntries() == 0)
        lk_warning << "Cannot open event display: detector is not set." << endl;

    if (gEve != nullptr) {
        lk_error << "gEve is nullptr" << endl;
        return;
    }

    TEveManager::Create(true, "V");
    auto eveEventManagerGlobal = new TEveEventManager("global");
    fEveEventManagerArray -> Add(eveEventManagerGlobal);
    gEve -> AddEvent(eveEventManagerGlobal);

    {
        Int_t dummy;
        UInt_t w, h;
        UInt_t wMax = 1200;
        UInt_t hMax = 720;
        Double_t r = (Double_t)wMax/hMax;
        gVirtualX -> GetWindowSize(gClient -> GetRoot() -> GetId(), dummy, dummy, w, h);

        if (w > wMax) {
            w = wMax;
            h = hMax;
        } else
            h = (Int_t)(w/r);

        gEve -> GetMainWindow() -> Resize(w, h);
    }

    gEve -> GetDefaultGLViewer() -> SetClearColor(kWhite);

    TGeoNode* geoNode = gGeoManager -> GetTopNode();
    TEveGeoTopNode* topNode = new TEveGeoTopNode(gGeoManager, geoNode, 1, 3, 10000);
    gEve -> AddGlobalElement(topNode);

    gEve -> FullRedraw3D(kTRUE);

    //gEve -> GetDefaultViewer() -> GetGLViewer() -> SetClearColor(kWhite);

    //return;

    /*
       gEve -> GetBrowser() -> SetTabTitle("3D", TRootBrowser::kRight);

       auto slotOv = TEveWindow::CreateWindowInTab(gEve -> GetBrowser() -> GetTabRight()); slotOv -> SetElementName("Overview Slot");
       auto packOv = slotOv -> MakePack(); packOv -> SetElementName("Overview Pack");

    // 1st Row
    auto slotPA = packOv -> NewSlot();
    auto packPA = slotPA -> MakePack();

    // Planes in 1st Row
    packPA -> SetHorizontal();
    for (auto iPlane = 0; iPlane < fDetectorSystem -> GetNumPlanes(); iPlane++) {
    auto slotPlane = packPA -> NewSlot(); slotPlane -> SetElementName(Form("Plane%d Slot", iPlane));
    auto ecvsPlane = new TRootEmbeddedCanvas();
    auto framPlane = slotPlane -> MakeFrame(ecvsPlane); framPlane -> SetElementName(Form("Detector Plane%d Frame", iPlane));

    TCanvas *cvs = ecvsPlane -> GetCanvas();
    fCvsDetectorPlaneArray -> Add(cvs);
    cvs -> cd();
    fDetectorSystem -> GetDetectorPlane(iPlane) -> GetHist(1) -> Draw("col");
    }

    // 2nd Row
    packOv -> SetVertical();
    auto slotCh = packOv -> NewSlotWithWeight(.35); slotCh -> SetElementName("Channel Buffer Slot");
    auto ecvsCh = new TRootEmbeddedCanvas();
    auto frameCh = slotCh -> MakeFrame(ecvsCh); frameCh -> SetElementName("Channel Buffer Frame");
    fCvsChannelBuffer = ecvsCh -> GetCanvas();

    gEve -> GetBrowser() -> GetTabRight() -> SetTab(1);
     */

    gEve -> GetBrowser() -> HideBottomTab();
    gEve -> ElementSelect(gEve -> GetCurrentEvent());
    gEve -> GetWindowManager() -> HideAllEveDecorations();
}

void LKEveTask::SetEveLineAtt(TEveElement *el, TString branchName)
{
    TString colorPar = Form("eveLineColor__%s",branchName.Data());
    if (fPar->CheckPar(colorPar)) {
        auto color = fPar -> GetParColor(colorPar);
        ((TEveLine *) el) -> SetLineColor(color);
    }

    TString widthPar = Form("eveLineWidth__%s",branchName.Data());
    if (fPar->CheckPar(widthPar)) {
        auto width = fPar -> GetParInt(widthPar);
        ((TEveLine *) el) -> SetLineWidth(width);
    }
}

void LKEveTask::SetEveMarkerAtt(TEveElement *el, TString branchName)
{
    TString colorPar = Form("eveMarkerColor__%s",branchName.Data());
    if (fPar->CheckPar(colorPar)) {
        auto color = fPar -> GetParColor(colorPar);
        ((TEvePointSet *) el) -> SetMarkerColor(color);
    }

    TString sizePar = Form("eveMarkerSize__%s",branchName.Data());
    if (fPar->CheckPar(sizePar)) {
        auto size = fPar -> GetParSize(sizePar);
        ((TEvePointSet *) el) -> SetMarkerSize(size);
    }
}

bool LKEveTask::SelectTrack(LKTracklet *tracklet)
{
    bool isGood = 1;
    if (fSelTrkIDs.size()!=0) { isGood=0; for (auto id:fSelTrkIDs) { if (tracklet->GetTrackID()==id)  { isGood=1; break; }}} if (!isGood) return false;
    if (fIgnTrkIDs.size()!=0) { isGood=1; for (auto id:fIgnTrkIDs) { if (tracklet->GetTrackID()==id)  { isGood=0; break; }}} if (!isGood) return false;
    if (fSelPntIDs.size()!=0) { isGood=0; for (auto id:fSelPntIDs) { if (tracklet->GetParentID()==id) { isGood=1; break; }}} if (!isGood) return false;
    if (fIgnPntIDs.size()!=0) { isGood=1; for (auto id:fIgnPntIDs) { if (tracklet->GetParentID()==id) { isGood=0; break; }}} if (!isGood) return false;
    if (fSelPDGs.size()!=0)   { isGood=0; for (auto id:fSelPDGs)   { if (tracklet->GetPDG()==id)      { isGood=1; break; }}} if (!isGood) return false;
    if (fIgnPDGs.size()!=0)   { isGood=1; for (auto id:fIgnPDGs)   { if (tracklet->GetPDG()==id)      { isGood=0; break; }}} if (!isGood) return false;
    //if (fSelMCIDs.size()!=0)  { isGood=0; for (auto id:fSelMCIDs)  { if (tracklet->GetMCID()==id)     { isGood=1; break; }}} if (!isGood) return false;
    //if (fIgnMCIDs.size()!=0)  { isGood=1; for (auto id:fIgnMCIDs)  { if (tracklet->GetMCID()==id)     { isGood=0; break; }}} if (!isGood) return false;
    return true;
}

bool LKEveTask::SelectHit(LKHit *hit)
{
    bool isGood = 1;
    if (fSelHitPntIDs.size()!=0) { isGood=0; for (auto id:fSelHitPntIDs) { if (hit->GetTrackID()==id) { isGood=1; break; }}} if (!isGood) { hit->SetSortValue(-1); return false; }
    if (fIgnHitPntIDs.size()!=0) { isGood=1; for (auto id:fIgnHitPntIDs) { if (hit->GetTrackID()==id) { isGood=0; break; }}} if (!isGood) { hit->SetSortValue(-1); return false; }
    //if (fSelMCIDs.size()!=0)     { isGood=0; for (auto id:fSelMCIDs)     { if (hit->GetMCID()==id)    { isGood=1; break; }}} if (!isGood) { hit->SetSortValue(-1); return false; }
    //if (fIgnMCIDs.size()!=0)     { isGood=1; for (auto id:fIgnMCIDs)     { if (hit->GetMCID()==id)    { isGood=0; break; }}} if (!isGood) { hit->SetSortValue(-1); return false; }
    return true;
}

void LKEveTask::AddEveElementToEvent(LKContainer *eveObj, bool permanent)
{
    if (eveObj -> IsEveSet()) {
        TEveElement *eveSet = eveObj -> CreateEveElement();
        eveObj -> AddToEveSet(eveSet, fEveScale);
        AddEveElementToEvent(eveSet, permanent);
    }
    else {
        TEveElement *eveElement = eveObj -> CreateEveElement();
        eveObj -> SetEveElement(eveElement, fEveScale);
        AddEveElementToEvent(eveElement, permanent);
    }
}

void LKEveTask::AddEveElementToEvent(TEveElement *element, bool permanent)
{
    ((TEveEventManager *) fEveEventManagerArray -> At(0)) -> AddElement(element);
    if (permanent) fPermanentEveElementList.push_back(element);
    gEve -> Redraw3D();
}
