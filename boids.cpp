#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "boids.h"

float Boid::Range_FAttract = 30.0f;
float Boid::Range_FRepel = 20.0f;
float Boid::Range_FAlign = 5.0f;
float Boid::FAttract_Vmax = 5.0f;
float Boid::FAttract_Factor = 4.0f;
float Boid::FRepel_Factor = 2.0f;
float Boid::FAlign_Factor = 2.0f;

Boid::Boid() {
    pNode = nullptr;
    pObject = nullptr;
    pRigidBody = nullptr;
    pCollisionShape = nullptr;
}

void Boid::Initialise(ResourceCache *pRes, Scene *pScene) {
    pNode = pScene->CreateChild("Boid");

    pObject = pNode->CreateComponent<StaticModel>();
    pObject->SetModel(pRes->GetResource<Model>("Models/Cone.mdl"));
    pObject->SetMaterial(pRes->GetResource<Material>("Materials/Stone.xml"));
    pObject->SetCastShadows(true);

    pRigidBody = pNode->CreateComponent<RigidBody>();
    pRigidBody->SetUseGravity(false);
    pRigidBody->SetPosition(Vector3(Random(40.0f) - 20.0f, 0.0f, Random(40.0f) - 20.0f));

    pCollisionShape = pNode->CreateComponent<CollisionShape>();
    pCollisionShape->SetTriangleMesh(pObject->GetModel(), 0);
}
void Boid::ComputeForce(Boid *pBoidList) {
    Vector3 CoM;
    int n = 0;

    force = Vector3(0, 0, 0);

    for (int i = 0; i < NumBoids; i++) {
        if (this == &pBoidList[i]) continue;

        Vector3 sep = pRigidBody->GetPosition() - pBoidList[i].pRigidBody->GetPosition();
        float d = sep.Length();

        if (d < Range_FAttract) {
            CoM += pBoidList[i].pRigidBody->GetPosition();
            n++;
        }
    }

    if (n > 0) {
        CoM /= n;
        Vector3 dir = (CoM - pRigidBody->GetPosition()).Normalized();
        Vector3 vDesired = dir * FAttract_Vmax;
        force += (vDesired - pRigidBody->GetLinearVelocity()) * FAttract_Factor;
    }
}
void Boid::Update(float tm) {

}

void BoidSet::Initialise(ResourceCache *pRes, Scene *pScene) {
    for (int i = 0; i < NumBoids; i++) {
        boidList[i].Initialise(pRes, pScene);
    }
}

void BoidSet::Update(float tm) {
    for (int i = 0; i < NumBoids; i++) boidList[i].ComputeForce(&boidList[0]);
}
