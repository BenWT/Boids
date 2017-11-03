#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "Missile.h"

Missile::Missile() {
    pNode = nullptr;
    pObject = nullptr;
    pRigidBody = nullptr;
    pCollisionShape = nullptr;
    timer = 0.0f;
}

void Missile::Initialise(ResourceCache *pRes, Scene *pScene) {
    pNode = pScene->CreateChild("Missile");
    pNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 15.0f, Random(40.0f) - 20.0f));

    pObject = pNode->CreateComponent<StaticModel>();
    pObject->SetModel(pRes->GetResource<Model>("Models/Sphere.mdl"));
    pObject->SetMaterial(pRes->GetResource<Material>("Materials/Stone.xml"));
    pObject->SetCastShadows(true);

    pRigidBody = pNode->CreateComponent<RigidBody>();
    pRigidBody->SetUseGravity(false);
    pRigidBody->SetMass(1.0f);
    pRigidBody->SetCollisionLayer(1);

    pCollisionShape = pNode->CreateComponent<CollisionShape>();
    pCollisionShape->SetTriangleMesh(pObject->GetModel(), 0);

    pNode->SetEnabled(false);
}

void Missile::DoShoot(Node *pCamera) {
    pNode->SetEnabled(true);
    timer = 0.0f;

    pRigidBody->SetPosition(pCamera->GetPosition());
    pRigidBody->SetLinearVelocity(pCamera->GetDirection().Normalized() * 20.0f);
}

void Missile::Update(float tm) {
    if (pNode->IsEnabled() == true) {
        timer += tm;

        if (timer > 5.0f) {
            pNode->SetEnabled(false);
            timer = 0.0f;
        }
    } else {
        pRigidBody->SetPosition(Vector3(0.0f, -50.0f, 0.0f));
        pRigidBody->SetLinearVelocity(Vector3(0.0f, 0.0f, 0.0f));
    }
}

MissileSet::MissileSet() {
    canShoot = true;

    for (int i = 0; i < 25; i++) {
        Missile m;
        missileList.Push(m);
    }
}
void MissileSet::Initialise(ResourceCache *pRes, Scene *pScene) {
    for (Vector<Missile>::Iterator m = missileList.Begin(); m < missileList.End(); m++) {
        m->Initialise(pRes, pScene);
    }
}
void MissileSet::DoShoot(Node *pCamera) {
    bool didShoot = false;

    for (Vector<Missile>::Iterator m = missileList.Begin(); m < missileList.End(); m++) {
        if (m->pNode->IsEnabled() == false) {
            didShoot = true;
            m->DoShoot(pCamera);
            canShoot = false;
            break;
        }
    }
}
void MissileSet::Update(float tm) {
    if (canShoot == false) {
        timer += tm;

        if (timer > 0.25f) {
            canShoot = true;
            timer = 0.0f;
        }
    }

    for (Vector<Missile>::Iterator m = missileList.Begin(); m < missileList.End(); m++) {
        m->Update(tm);
    }
}
