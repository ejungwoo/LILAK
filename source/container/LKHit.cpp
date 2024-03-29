#include "LKHit.hpp"
#include "LKTracklet.hpp"
#ifdef ACTIVATE_EVE
#include "TEvePointSet.h"
#endif
#include "TMath.h"
#include <iostream>
#include <iomanip>

ClassImp(LKHit)

void LKHit::Clear(Option_t *option)
{
    LKWPoint::Clear(option);

    fHitID = -1;
    fTrackID = -1;
    fAlpha = -999;
    fDX = -999;
    fDY = -999;
    fDZ = -999;
    fSortValue = 0;

    //fHitArray.Clear("C");
    fTrackCandArray.clear();
}

void LKHit::Print(Option_t *option) const
{
    lx_info << "HID,TID,X,Y,Z,W: "
        << setw(4)  << fHitID
        << setw(4)  << fTrackID << " |"
        << setw(12) << fX
        << setw(12) << fY
        << setw(12) << fZ << " |"
        << setw(12) << fW << endl;
}

void LKHit::Copy(TObject &obj) const
{
    LKWPoint::Copy(obj);
    auto hit = (LKHit &) obj;

    hit.SetHitID(fHitID);
    hit.SetTrackID(fTrackID);

    /* TODO
       auto numHits = fHitArray.GetNumHits();
       for (auto i = 0; i < numHits; ++i)
       hit.AddHit(fHitArray.GetHit(i));
     */
}

void LKHit::CopyFrom(LKHit *hit)
{
    fDX        = hit -> GetDX       ();
    fDY        = hit -> GetDY       ();
    fDZ        = hit -> GetDZ       ();
    fX         = hit -> GetX        ();
    fY         = hit -> GetY        ();
    fZ         = hit -> GetZ        ();
    fW         = hit -> GetCharge   ();
    fAlpha     = hit -> GetAlpha    ();
    fHitID     = hit -> GetHitID    ();
    fSortValue = hit -> GetSortValue();
    fTrackID   = hit -> GetTrackID  ();
}

void LKHit::SetSortValue(Double_t val) { fSortValue = val; }
Double_t LKHit::GetSortValue() const { return fSortValue; }

void LKHit::SetSortByX(bool sortEarlierIfSmaller) {
    if (sortEarlierIfSmaller) fSortValue =  fX;
    else                      fSortValue = -fX;
}

void LKHit::SetSortByY(bool sortEarlierIfSmaller) {
    if (sortEarlierIfSmaller) fSortValue =  fY;
    else                      fSortValue = -fY;
}

void LKHit::SetSortByZ(bool sortEarlierIfSmaller) {
    if (sortEarlierIfSmaller) fSortValue =  fZ;
    else                      fSortValue = -fZ;
}

void LKHit::SetSortByCharge(bool sortEarlierIfSmaller)
{
    if (sortEarlierIfSmaller) fSortValue =  fW;
    else                      fSortValue = -fW;
}

void LKHit::SetSortByDistanceTo(TVector3 point, bool sortEarlierIfCloser)
{
    auto dist = (point - GetPosition()).Mag();
    if (sortEarlierIfCloser) fSortValue =  dist;
    else                     fSortValue = -dist;
}

Bool_t LKHit::IsSortable() const { return true; }

Int_t LKHit::Compare(const TObject *obj) const
{
    auto hitCompare = (LKHit *) obj;

    int sortEarlier = 1;
    int sortLatter = -1;
    int sortSame = 0;

    if (fSortValue < hitCompare -> GetSortValue()) return sortEarlier;
    else if (fSortValue > hitCompare -> GetSortValue()) return sortLatter;

    return sortSame;
}

/*
void LKHit::PropagateMC()
{
    if (fHitArray.GetNumHits()==0)
        return;

    vector<Int_t> mcIDs;
    vector<Int_t> counts;

    TIter next(&fHitArray);
    LKHit *component;
    while ((component = (LKHit *) next()))
    {
        auto mcIDCoponent = component -> GetMCID();

        Int_t numMCIDs = mcIDs.size();
        Int_t idxFound = -1;
        for (Int_t idx = 0; idx < numMCIDs; ++idx) {
            if (mcIDs[idx] == mcIDCoponent) {
                idxFound = idx;
                break;
            }
        }
        if (idxFound == -1) {
            mcIDs.push_back(mcIDCoponent);
            counts.push_back(1);
        }
        else {
            counts[idxFound] = counts[idxFound] + 1;
        }
    }

    auto maxCount = 0;
    for (auto count : counts)
        if (count > maxCount)
            maxCount = count;

    vector<Int_t> iIDCandidates;
    for (auto iID = 0; iID < Int_t(counts.size()); ++iID)
        if (counts[iID] == maxCount)
            iIDCandidates.push_back(iID);


    //TODO @todo
    if (iIDCandidates.size() == 1)
    {
        auto iID = iIDCandidates[0];
        auto mcIDFinal = mcIDs[iID];

        auto errorFinal = 0.;
        next.Begin();
        while ((component = (LKHit *) next()))
            if (component -> GetMCID() == mcIDFinal)
                errorFinal += component -> GetMCError();

        errorFinal = errorFinal/counts[iID];
        Double_t purity = Double_t(counts[iID])/fHitArray.GetNumHits();
        SetMCTag(mcIDFinal, errorFinal, purity);
    }
    else
    {
        auto mcIDFinal = 0;
        auto errorFinal = DBL_MAX;
        Double_t purity = -1;

        for (auto iID : iIDCandidates) {
            auto mcIDCand = mcIDs[iID];

            auto errorCand = 0.;
            next.Begin();
            while ((component = (LKHit *) next()))
                if (component -> GetMCID() == mcIDCand)
                    errorCand += component -> GetMCError();
            errorCand = errorCand/counts[iID];

            if (errorCand < errorFinal) {
                mcIDFinal = mcIDCand;
                errorFinal = errorCand;
                purity = Double_t(counts[iID])/fHitArray.GetNumHits();
            }
        }
        SetMCTag(mcIDFinal, errorFinal, purity);
    }
}
*/

void LKHit::SetHitID(Int_t id) { fHitID = id; }
void LKHit::SetTrackID(Int_t id) { fTrackID = id; }
void LKHit::SetAlpha(Double_t a) { fAlpha = a; }
void LKHit::SetDPosition(TVector3 dpos) { fDX = dpos.X(); fDY = dpos.Y(); fDZ = dpos.Z(); }
void LKHit::SetDX(Double_t dx) { fDX = dx; }
void LKHit::SetDY(Double_t dy) { fDY = dy; }
void LKHit::SetDZ(Double_t dz) { fDZ = dz; }
void LKHit::SetX(Double_t x) { fX = x; }
void LKHit::SetY(Double_t y) { fY = y; }
void LKHit::SetZ(Double_t z) { fZ = z; }
void LKHit::SetCharge(Double_t charge) { fW = charge; }

/*
void LKHit::AddHit(LKHit *hit)
{
    fHitArray.AddHit(hit);
    fX = fHitArray.GetMeanX();
    fY = fHitArray.GetMeanY();
    fZ = fHitArray.GetMeanZ();
    fW = fHitArray.GetW();
}

void LKHit::RemoveHit(LKHit *hit)
{
    fHitArray.RemoveHit(hit);
    fX = fHitArray.GetMeanX();
    fY = fHitArray.GetMeanY();
    fZ = fHitArray.GetMeanZ();
    fW = fHitArray.GetW();
}
*/

Int_t LKHit::GetHitID()   const { return fHitID; }
Int_t LKHit::GetTrackID() const { return fTrackID; }
Double_t LKHit::GetAlpha()   const { return fAlpha; }
TVector3 LKHit::GetDPosition() const { return TVector3(fDX,fDY,fDZ); }
Double_t LKHit::GetDX()      const { return fDX; }
Double_t LKHit::GetDY()      const { return fDY; }
Double_t LKHit::GetDZ()      const { return fDZ; }
Double_t LKHit::GetX()       const { return fX; }
Double_t LKHit::GetY()       const { return fY; }
Double_t LKHit::GetZ()       const { return fZ; }
Double_t LKHit::GetCharge()  const { return fW; }


//TVector3 LKHit::GetMean()          const { return fHitArray.GetMean();          }
//TVector3 LKHit::GetVariance()      const { return fHitArray.GetVariance();      }
//TVector3 LKHit::GetCovariance()    const { return fHitArray.GetCovariance();    }
//TVector3 LKHit::GetStdDev()        const { return fHitArray.GetStdDev();        }
//TVector3 LKHit::GetSquaredMean()   const { return fHitArray.GetSquaredMean();   }
//TVector3 LKHit::GetCoSquaredMean() const { return fHitArray.GetCoSquaredMean(); }


std::vector<Int_t> *LKHit::GetTrackCandArray() { return &fTrackCandArray; }
Int_t LKHit::GetNumTrackCands() { return fTrackCandArray.size(); }
void LKHit::AddTrackCand(Int_t id) { fTrackCandArray.push_back(id); }

void LKHit::RemoveTrackCand(Int_t trackID)
{
    Int_t n = fTrackCandArray.size();
    for (auto i = 0; i < n; i++) {
        if (fTrackCandArray[i] == trackID) {
            fTrackCandArray.erase(fTrackCandArray.begin()+i); 
            return;
        }
    }
    fTrackCandArray.push_back(-1);
}

#ifdef ACTIVATE_EVE
bool LKHit::DrawByDefault() { return true; }
bool LKHit::IsEveSet() { return true; }

TEveElement *LKHit::CreateEveElement()
{
    auto pointSet = new TEvePointSet("Hit");
    pointSet -> SetMarkerColor(kAzure-8);
    pointSet -> SetMarkerStyle(20);
    pointSet -> SetMarkerSize(0.5);
    //pointSet -> SetMarkerColor(kBlack);
    //pointSet -> SetMarkerSize(1.0);
    //pointSet -> SetMarkerStyle(38);

    return pointSet;
}

void LKHit::SetEveElement(TEveElement *, Double_t)
{
}

void LKHit::AddToEveSet(TEveElement *eveSet, Double_t scale)
{
    auto pointSet = (TEvePointSet *) eveSet;
    pointSet -> SetNextPoint(scale*fX, scale*fY, scale*fZ);
}
#endif
