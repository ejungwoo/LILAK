#ifndef LKSTEPPINGACTION_HH
#define LKSTEPPINGACTION_HH

#include "LKG4RunManager.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "globals.hh"

class LKSteppingAction : public G4UserSteppingAction
{
    public:
        LKSteppingAction();
        LKSteppingAction(LKG4RunManager *man);
        virtual ~LKSteppingAction() {}

        virtual void UserSteppingAction(const G4Step*);

    private:
        LKG4RunManager *fRunManager = nullptr;
};

#endif
