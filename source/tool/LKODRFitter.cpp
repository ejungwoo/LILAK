#include "LKLogger.hpp"
#include "LKODRFitter.hpp"
#include "LKLogger.hpp"

#include <iostream>
#include <iomanip>
using namespace std;

ClassImp(LKODRFitter);

LKODRFitter* LKODRFitter::fInstance = nullptr;

LKODRFitter* LKODRFitter::GetFitter() {
  if (fInstance != nullptr)
    return fInstance;
  return new LKODRFitter();
}

LKODRFitter::LKODRFitter()
{
  fInstance = this;
  fNormal = new TVectorD(3);
  fMatrixA = new TMatrixD(3,3);
  fEigenValues = new TVectorD(3);
  fEigenVectors = new TMatrixD(3,3);

  Reset();
}

LKODRFitter::~LKODRFitter()
{
}

void LKODRFitter::Reset()
{
  fXCentroid = 0;
  fYCentroid = 0;
  fZCentroid = 0;

  fNumPoints = 0;
  fWeightSum = 0;
  fSumOfPC2 = 0;

  for (Int_t i = 0; i < 3; i++)
    for (Int_t j = 0; j < 3; j++)
      (*fMatrixA)[i][j] = 0;

  fRMSLine = -1;
  fRMSPlane = -1;
}

void LKODRFitter::Print()
{
  lx_info << "Number of points:       " << fNumPoints << endl;
  lx_info << "Sum of weight:          " << fWeightSum << endl;
  lx_info << "Sum of sqrt[(x-<x>)^2]: " << fSumOfPC2 << endl;
  lx_info << "<x> = " << fXCentroid << endl;
  lx_info << "<y> = " << fYCentroid << endl;
  lx_info << "<z> = " << fZCentroid << endl;

  if (fRMSLine!=-1 && fRMSPlane!=-1) {
    lx_info << "Mat. A=|" << setw(15) << (*fMatrixA)[0][0] << setw(15) << (*fMatrixA)[0][1] << setw(15) << (*fMatrixA)[0][2] << "|" << endl;
    lx_info << "       |" << setw(15) << (*fMatrixA)[1][0] << setw(15) << (*fMatrixA)[1][1] << setw(15) << (*fMatrixA)[1][2] << "|" << endl;
    lx_info << "       |" << setw(15) << (*fMatrixA)[2][0] << setw(15) << (*fMatrixA)[2][1] << setw(15) << (*fMatrixA)[2][2] << "|" << endl;

    auto vec0 = TMatrixDColumn((*fEigenVectors), 0);
    auto vec1 = TMatrixDColumn((*fEigenVectors), 1);
    auto vec2 = TMatrixDColumn((*fEigenVectors), 2);
    lx_info << "Eigen val./vec. 0: " << (*fEigenValues)[0] << " / (" << vec0[0] << "," << vec0[1] << "," << vec0[2] << ")" << endl;
    lx_info << "Eigen val./vec. 1: " << (*fEigenValues)[1] << " / (" << vec1[0] << "," << vec1[1] << "," << vec1[2] << ")" << endl;
    lx_info << "Eigen val./vec. 2: " << (*fEigenValues)[2] << " / (" << vec2[0] << "," << vec2[1] << "," << vec2[2] << ")" << endl;

    lx_info << "RMS(line)  = " << fRMSLine << endl;
    lx_info << "RMS(plane) = " << fRMSPlane << endl;
  }
}

void LKODRFitter::SetCentroid(Double_t x, Double_t y, Double_t z)
{
  fXCentroid = x;
  fYCentroid = y;
  fZCentroid = z;
}

void LKODRFitter::PreAddPoint(Double_t x, Double_t y, Double_t z, Double_t w)
{
  auto weightSumNew = fWeightSum + w;

  fXCentroid = (fWeightSum*fXCentroid + w*x) / weightSumNew;
  fYCentroid = (fWeightSum*fYCentroid + w*y) / weightSumNew;
  fZCentroid = (fWeightSum*fZCentroid + w*z) / weightSumNew;
  fWeightSum = weightSumNew;
}

void LKODRFitter::AddPoint(Double_t x, Double_t y, Double_t z, Double_t w)
{
  Double_t dX = x - fXCentroid;
  Double_t dY = y - fYCentroid;
  Double_t dZ = z - fZCentroid;

  Double_t wx2 = w * dX * dX;
  Double_t wy2 = w * dY * dY;
  Double_t wz2 = w * dZ * dZ;

  (*fMatrixA)[0][0] += wx2;
  (*fMatrixA)[0][1] += w * dX * dY;
  (*fMatrixA)[0][2] += w * dX * dZ;

  (*fMatrixA)[1][1] += wy2;
  (*fMatrixA)[1][2] += w * dY * dZ;

  (*fMatrixA)[2][2] += wz2;

  if (fNumPoints==0) {
    fSumOfPC2 = wx2 + wy2 + wz2;
    fWeightSum = w;
  }
  else {
    fSumOfPC2 += wx2 + wy2 + wz2;
    fWeightSum += w;
  }
  fNumPoints++;
}

void LKODRFitter::SetMatrixA(
  Double_t c00, 
  Double_t c01, 
  Double_t c02,
  Double_t c11, 
  Double_t c12, 
  Double_t c22)
{
  (*fMatrixA)[0][0] = c00;
  (*fMatrixA)[0][1] = c01;
  (*fMatrixA)[0][2] = c02;

  (*fMatrixA)[1][1] = c11;
  (*fMatrixA)[1][2] = c12;

  (*fMatrixA)[2][2] = c22;

  fSumOfPC2 += c00 + c11 + c22;
}

void LKODRFitter::SetWeightSum(Double_t weightSum) { fWeightSum = weightSum; }
void LKODRFitter::SetNumPoints(Double_t numPoints) { fNumPoints = numPoints; }

bool LKODRFitter::Solve()
{
  (*fMatrixA)[1][0] = (*fMatrixA)[0][1];
  (*fMatrixA)[2][0] = (*fMatrixA)[0][2];
  (*fMatrixA)[2][1] = (*fMatrixA)[1][2];

  if ((*fMatrixA)[0][0] == 0 && (*fMatrixA)[1][1] == 0 && (*fMatrixA)[2][2] == 0)
    return false;

  (*fEigenVectors) = fMatrixA -> EigenVectors(*fEigenValues);
  return true;
}

void LKODRFitter::ChooseEigenValue(Int_t iEV)
{
  (*fNormal) = TMatrixDColumn((*fEigenVectors), iEV);

  fRMSLine = (fSumOfPC2 - (*fEigenValues)[iEV]) / (fWeightSum - 2*fWeightSum/fNumPoints);
  fRMSLine = TMath::Sqrt(fRMSLine);

  fRMSPlane = (*fEigenValues)[iEV] / (fWeightSum - 2*fWeightSum/fNumPoints);
  if (fRMSPlane < 0) fRMSPlane = 0;
  fRMSPlane = TMath::Sqrt(fRMSPlane);
}

bool LKODRFitter::FitLine()
{
  if (Solve() == false)
    return false;

  ChooseEigenValue(0);
  return true;
}

bool LKODRFitter::FitPlane()
{
  if (Solve() == false)
    return false;

  ChooseEigenValue(2);
  return true;
}

TVector3 LKODRFitter::GetCentroid()  { return TVector3(fXCentroid, fYCentroid, fZCentroid); }
TVector3 LKODRFitter::GetNormal()    { return TVector3((*fNormal)[0], (*fNormal)[1], (*fNormal)[2]); }
TVector3 LKODRFitter::GetDirection() { return TVector3((*fNormal)[0], (*fNormal)[1], (*fNormal)[2]); }
   Int_t LKODRFitter::GetNumPoints() { return fNumPoints; }
Double_t LKODRFitter::GetWeightSum() { return fWeightSum; }
Double_t LKODRFitter::GetRMSLine()   { return fRMSLine; }
Double_t LKODRFitter::GetRMSPlane()  { return fRMSPlane; }
