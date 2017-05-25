#include <json/json.hpp>
#include "Level.hxx"
#include <util/file.hxx>
#include <util/conversion.hxx>
#include <graphics/mesh.hxx>
#include <physics/Volume.hxx>
#include <ent/terrain.hxx>
#include <ent/water.hxx>
#include <ent/plane.hxx>
#include <world/world.hxx>
#include <physics/physics.hxx>

extern World* world;

using json = nlohmann::json;

Level::Level() {}

Level::Level(const char *path) {
    this->path = path;
}

Level::~Level() 
{ 
    this->unload();
}

//quaternion quatFromSTDVec(std::vector<float> v) {
//    return quaternion(v.at(0), v.at(1), v.at(2), v.at(3));
//}
//
//vec3 vec3FromSTDVec(std::vector<float> v) {
//    return vec3(v.at(0), v.at(1), v.at(2));
//}

Entity *entityFromJSON(int id, json j) {
    /* copy over entity data */
    std::string name = j["name"];
    std::vector<float> position = j["position"];
    vec3 real_position = convert::vec3FromSTDVec(position);
    std::vector<float> orientation = j["orientation"];
    quaternion realOrientation = convert::quatFromSTDVec(orientation);
    float tick_interval = j["tick_interval"];
    bool movable = j["movable"];
    bool drawable = j["drawable"];
    vec3 color = vec3(0,0,0);
    char *model_file = NULL;

    if (drawable) {
        std::vector<float> fakeColor = j["color"];
        color = convert::vec3FromSTDVec(fakeColor);
        std::string model_file_str = j["model"];
        model_file = strdup(model_file_str.c_str());
    }

    bool collidable = j["collidable"];
    std::string volume_type = "";
    json volume_data = {};

    /* Create entity */
    // this line probably should be edited in the future to reflect "not all entities are gadgets"
    Entity *retVal = new Gadget(id, real_position, realOrientation, name.c_str(), TYPE1, SPAWNED, tick_interval, color, model_file,"../assets/textures/wood1.png");
    if (movable) {
        float mass = j["mass"];
        retVal->setMass(mass);
        float dragCoef = j["dragCoef"];
        retVal->setPhysicsParams({dragCoef});
        std::vector<float> velocity = j["velocity"];
        retVal->vel(convert::vec3FromSTDVec(velocity));
    }
    if (collidable) {
        volume_type = j["volume-type"];
        volume_data = j["volume-data"];
        // initialize a sphere
        if (volume_type.compare("sphere") == 0) {
            std::vector<float> spherePos = volume_data["position"];
            vec3 sphereCenter = convert::vec3FromSTDVec(spherePos);
            float sphereRad = volume_data["radius"];
            retVal->setVolume(new SphereVolume(Volume::Pos(retVal), sphereRad));
        }
    }

    /* Free any memory used in object construction */
    /* TODO: We almost def have to free more stuff here */
    free(model_file);

    return retVal;
}

/* Checks if the entity should be deleted on world unload. Some things, like
submarines, stick around across levels. */
int Level::shouldDeleteOnUnload(Entity *entity)
{
    return !(entity->getEntityType() == TYPESUB);
}

/* Release all memeory associated with this level */
void Level::unload()
{
    for(auto ent : this->entities){
        //logln(LOGMEDIUM, "removing entity %d", ent.second->getID());
        if(this->shouldDeleteOnUnload(ent.second))
            delete(ent.second);
    }
}

/* Populate all fields of the class by loading them from a file. */
void Level::buildLevelFromFile() {
    if(!this->path){
        buildDemoLevel();
        return;
    }
    int id =0;
    char *raw = fileio::load_file(this->path);
    json raw_j = json::parse(raw);
    std::vector<json> entities = raw_j["entities"];

    std::vector<json>::iterator it = entities.begin();
    while (it != entities.end()) {
        Entity *ent = entityFromJSON(++id,*it);
        //logln(LOGMEDIUM, "adding an item called %s\n", ent->getName().c_str());
        this->addEntity(ent);
        ++it;
    }
    // todo: remove this. 
    Entity *cave = new Terrain(++id, vec3(), quaternion(), "canyon", TYPE1, SPAWNED, 1.f, vec3(1.f,0.8f,0.5f), "../assets/textures/moss1.png", "../assets/heightmaps/bump_bump.hmp");
    addEntity(cave);
    // cave->vel(vec3(0,8,0));
    cave->pos(vec3(-50,-40,-50));;
    cave->mass(99999);

     //create skybox
    Gadget *skybox = new Gadget(++id,vec3(0,0,0), quaternion(), strdup("sky"), TYPE1, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/sphere.obj","../assets/textures/wood1.png");
    this->setSkybox(skybox);

}

/* Generate a sequence of n regular hexagons with radius r, centered at center[i] for every i in [0,n]. */
void Level::generateDummyPath(float r, vec3 *centers, int n, int& cur_id)
{
    Track *track = new Track();
    this->track = track;

    int i;
    Hexagon *hex;
    SeekPoint *seek;
    for(i=0; i<n; i++){
        hex = new Hexagon(centers[i],r);
        seek = new SeekPoint(cur_id++, centers[i], quaternion(), "check", TYPECHECK, SPAWNED, 0.1f, hex, 1);
        seek->setMass(1);
        seek->setVelocity(vec3(0,0,0));
        track->addSeekPoint(seek);
        this->addEntity(seek);
    }
}

static Entity *planes[4];
static int nplanes =0;

/* DEMO LEVEL */
void Level::buildDemoLevel() 
{ 
  //create test objects
    // works between (-2.5, -4.5)
    vec3 cubePos[] = {vec3(0,14,0), vec3(5.f, 10, 5.5f), vec3(10,10,15), vec3(10,7,15), vec3(3, -13, 5),
        vec3(5, -40, 5)}; 
    vec3 cubeColor[] = {vec3(0.8,1,0), vec3(1,0,1), vec3(1,0,0), vec3(1,0,1), vec3(0,1,1), vec3(0,0,1)};
    int ncubes = 2, i;
    Entity * cubes[ncubes];

    int cur_id = 0;

    cubes[0] = new Gadget(cur_id++,cubePos[0], quaternion(), "sub", TYPE1, SPAWNED, 0.1f, cubeColor[0], "../assets/models/boxes.obj","../assets/textures/wood1.png");
    cubes[0]->setOrientation(angleAxis(3.1415f/2.f,vec3(0.f,1.f,0.f)));
    cubes[0]->setVolume(new CylinderVolume(Volume::Pos(cubes[0]),1.f,9.f,glm::rotate(glm::mat4(1),3.14159265f/2.f,glm::vec3(1,0,0))));
    cubes[0]->meshes.push_back(cubes[0]->getVolume()->collisionMesh());
    // cubes[0]->setVelocity(vec3(0,-6.5f,0));
    cubes[0]->setMass(1.f);

    cubes[1] = new Gadget(cur_id++,cubePos[1], quaternion(), "cube"+std::to_string(i), TYPE1, SPAWNED, 0.1f, cubeColor[1], "../assets/models/boxes.obj","../assets/textures/wood1.png");
    cubes[1]->setVolume(new CylinderVolume(Volume::Pos(cubes[1]),1.f,9.f,glm::rotate(glm::mat4(1),3.14159265f/2.f,vec3(1,0,0))));
    cubes[1]->meshes.push_back(cubes[1]->getVolume()->collisionMesh());
    // cubes[1]->setVelocity(vec3(0,-2,0));
    cubes[1]->setMass(1.f);
    cubes[1]->dragCoef(0.f);

    for(i=2; i<4; i++){
        cubes[i] = new Gadget(cur_id++,cubePos[i], quaternion(angleAxis(-1.f,normalize(vec3(0.2f,1,0.2)))), "cube"+std::to_string(i), TYPE1, SPAWNED, 0.1f, cubeColor[i], "../assets/models/boxes.obj","../assets/textures/wood1.png");
        cubes[i]->setVolume(new SphereVolume(Volume::Pos(cubes[i]),1.414));
        // cubes[i]->meshes.push_back(cubes[i]->getVolume()->collisionMesh());
        // cubes[i]->setVelocity(vec3(0,-3,0));
        cubes[i]->setMass(1.f);
        cubes[i]->dragCoef(0.0f);
    }


    Entity *e;
    e = new Gadget(cur_id++,vec3(-30,17,0), quaternion(angleAxis(1.f,normalize(vec3(0.4f,1,0.4)))), "monkey", TYPE1, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/plane-low-3.obj","../assets/textures/marble.png");
    addEntity(e);

    planes[0] = new Gadget(cur_id++,vec3(0,17,0), quaternion(), "box", TYPE1, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/plane-low-3.obj","../assets/textures/piper_diffuse.png");
    // planes[0]->setVolume(new SphereVolume(Volume::Pos(planes[0]),1.0));
    // planes[0]->vel(vec3(0,0,-15));

    nplanes = 1;
    // e->meshes.push_back(e->getVolume()->collisionMesh());
    addEntity(planes[0]);

    // cubes[2]->setVelocity(vec3(0,-2,0));
    cubes[2]->setMass(10);

    cubes[3]->setVelocity(vec3(0,0,0));
    cubes[3]->setMass(1);

    for(i=1; i<4; i++);
      // this->addEntity(cubes[i]);


    // for(int i=0;i<1;i++){
    //   Entity *e = new Terrain(cur_id++, vec3(), quaternion(), "canyon2", TYPE1, SPAWNED, 1.f, vec3(1.f,0.8f,0.5f), "../assets/textures/moss1.png", "../assets/heightmaps/bump_bump.hmp");
    //   e->mass(9999);
    //   addEntity(e);
    // }

    //create checkpoints
    /*Hexagon * hex1 = new Hexagon(vec3(-5,2,-3), vec3(-5,-2,-3), vec3(0,5,0), vec3(0,-5,0), vec3(5,2,3), vec3(5,-2,3));
    SeekPoint *seek1 = new SeekPoint(cur_id++, vec3(5,6,5), quaternion(), "check", TYPECHECK, SPAWNED, 0.1f, hex1, 1);
    seek1->setMass(1);
    seek1->setVelocity(vec3(0,0,0));
    this->addEntity(seek1);*/

    // Submarine * sub = new Submarine(cur_id++,vec3(10,10,10), glm::angleAxis(1.74f, vec3(0, -1, 0)), strdup("sub1"), TYPESUB, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/cube.obj");
    // sub->mass(1.0);
    // sub->dragCoef(0.3); 
    // SubmarineAI * ai1 = new SubmarineAI();
    // ai1->bindToSubAct((SubmarineActuator*)sub->getActuator()); 
    // this->addEntity(sub);
    // this->addAI(ai1, 0.1);


    int ncenters = 6;
    vec3 centers[ncenters] = {vec3(5,5,0),vec3(5,5,5),vec3(5,5,10),vec3(7,5,15),vec3(9,5,20),vec3(9,8,25)};
    // this->generateDummyPath(3, centers, ncenters, cur_id);

    Entity *water;

    water = new Water(cur_id++, "water1", vec3(-800,0,-800), vec2(150,150), vec3(1600,1,1600),vec3(1.f,0.8f,0.5f));
    addEntity(water);
    // water = new Water(cur_id++, "water2", vec3(-100,0,0), vec2(100,100), vec3(1.f,0.8f,0.5f));
    // addEntity(water);

    // Entity *cave = new Terrain(cur_id++, vec3(), quaternion(), "canyon2", TYPE1, SPAWNED, 1.f, vec3(1.f,0.8f,0.5f), "../assets/textures/moss1.png", "../assets/heightmaps/bump_bump.hmp");
    Entity *cave = new Gadget(cur_id++,vec3(0,-40,0), quaternion(), "cube"+std::to_string(i), TYPE1, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/crate.obj","../assets/textures/wood1.png");
    cave->mass(9999);
    cave->pos(vec3(0,-40,0));
    // this->addEntity(cave);

    //create skybox
    Gadget *skybox = new Gadget(cur_id++,vec3(0,0,0), quaternion(), "sky", TYPE1, SPAWNED, 0.1f, vec3(1,1,1), "../assets/models/skydome.obj","../assets/textures/sky_redsunset.png");
    this->setSkybox(skybox);

    //logln(LOGHIGH,"built level.");
//
}

/* Update the data for an entity based on a CODE_OBJECT_CHANGE message */
void Level::upEntData(posUpBuf *info)
{
  // Get the entity for this ID
  Entity *ent = this->getEntityByID(info->id);
  if(ent == NULL){
    //logln(LOGERROR, "%s %d\n", "Null entity in upEntData", info->id);
    return;
  }

  ent->overwrite(info);
}

/* Spawn or despawn an object. Returns the old EntityStatus */
EntityStatus Level::changeObjectSpawn(int id, EntityStatus n) { /*TODO*/return SPAWNED; }


/* Get the entity with the given ID */
Entity * Level::getEntityByID(int id)
{
  if(!this->entityExists(id))
    return NULL;
  return this->entities[id];
}

/* Returns 1 if an entity with id exists in our entity list,
0 otherwise */
int Level::entityExists(int id)
{
  return this->entities.find(id) != entities.end();
}

/* Add this entity to the entity list. This function checks that an entity
with the current ID does not already exist. Returns 1 if the entity was
added, 0 if it couldn't due to the ID already being taken. Since we
identify entities by ID, we can't let one entity "overwrite" another. */
int Level::addEntity(Entity *entity)
{ 
  if(this->entityExists(entity->getID()))
    return 0;
  this->entities[entity->getID()] = entity;
  return 1;
}

//Returns 0 if successful, -1 if the element can't be found
int Level::removeEntity(Entity *entity)
{   
    //TODO
    return -1;
}



void Level::addAI(AI *ai, float tickrate)
{   
    AI_entry entry;
    entry.ai = ai;
    entry.tickrate = tickrate;
    entry.time_left = tickrate;
    ais.push_back(entry);
}
//Returns 0 if successful, -1 if the element can't be found
int Level::removeAI(AI *ai)
{   
    for (std::vector<AI_entry>::const_iterator it = ais.begin() ; it != ais.end(); ++it) {
        if(it->ai == ai) {
            ais.erase(it);
            return 0;
        }
    }
    
    return -1;
}



/* Using the given server, broadcast position updates to all clients. */
void Level::sendPosUps(Server *server){
  for (auto entry : this->entities){
        auto entity = entry.second;
        bool should = entity->isShouldSendUpdate();
        if(should){
            //fprintf(stderr,"%s: %s\n",entry.second->getName().c_str(),should?"1":"-");
            message* m = entity->prepareMessageSegment();
            server->broadcast(m);
            deleteMessage(m);
        }
    }
}


void Level::renderAll(View *view, RendererList rs, int passes){
    WaterRenderer* ws = (WaterRenderer*)rs.water;
    int width, height;

    // todo which one do we choose?
    // glfwGetWindowSize(world->window, &width, &height);
    glfwGetFramebufferSize(world->window, &width, &height);

    // ^^^^ todo 

    ws->screensize=vec2(width,height);

    ws->time += 0.04f;
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    static GLuint framebuffer = 0;
    if(!framebuffer){
        GLenum err;
        glActiveTexture(GL_TEXTURE0 + 0);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);


        // "Bind" the newly created texture : all future texture functions will modify this texture

        // Give an empty image to OpenGL ( the last "0" ). Poor filtering (needed!)


        // The depth buffer
        GLuint depthrenderbuffer;
        glGenRenderbuffers(1, &depthrenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1600, 1200);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

// texReflDepth
        // Set "out_texture" as our colour attachement #0
        while((err = glGetError()) != GL_NO_ERROR){
          logln(LOGHIGH,"OpenGL Error 0: %d %s",err,"");
        }
        GLuint out_texture;
        glGenTextures(1, &out_texture);
        glBindTexture(GL_TEXTURE_2D, out_texture);
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, 1600, 1200, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_texture, 0);
        while((err = glGetError()) != GL_NO_ERROR){
          logln(LOGHIGH,"OpenGL Error 1: %d %s",err,"");
        }
        glGenTextures(1, &(rs.water->texReflDepth));
        glBindTexture(GL_TEXTURE_2D, rs.water->texReflDepth);
        glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT24, 1600, 1200, 0,GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rs.water->texReflDepth, 0);
        while((err = glGetError()) != GL_NO_ERROR){
          logln(LOGHIGH,"OpenGL Error: %d %s",err,"");
        }
        // Set the list of draw buffers.
        GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
        glDrawBuffers(2, DrawBuffers); // "1" is the size of DrawBuffers

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            logln(LOGHIGH,"Error in Framebuffer creation");
        rs.water->reflection_texture = out_texture;

        glBindTexture(GL_TEXTURE_2D,out_texture);

    }
    if(passes==1){
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        rs.ent->reflectY = true;
        renderAll(view,rs,0);
        rs.ent->reflectY = false;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else{
      glClearColor(1,1,1,1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
      // return;
    }
    rs.ent->enable();
    rs.ent->modeSkybox();
    rs.ent->render(view, this->skybox);
    rs.ent->modeNormal();
    for (auto entry : this->entities) {
        auto entity = entry.second;
        //logln(LOGMEDIUM, "rendering entity %d with %p\n", entity->getID(), r);
        if(entity->isDrawable()){
            switch (entity->getEntityType()){
                case WATER:
                    if(passes){
                        glActiveTexture(GL_TEXTURE0 + 1);
                        glBindTexture(GL_TEXTURE_2D,rs.water->reflection_texture);
                        glActiveTexture(GL_TEXTURE0 + 0);
                        glBindTexture(GL_TEXTURE_2D,rs.water->texReflDepth);
                        rs.water->enable();
                        rs.water->render(view, entity);
                        glActiveTexture(GL_TEXTURE0 + 0);
                    }
                    break;
                default:
                    rs.ent->enable();
                    rs.ent->render(view, entity);

            }
        }
    }
}

/* Skybox */
void Level::setSkybox(Gadget *skybox)
{
    this->skybox = skybox;
}


void Level::updateLevel(float dt) {
    updateAIs(dt);
    //printf("Done updating AIs");
    physicsTick(dt);
    //printf("Done updating level\n");
}
void Level::interpolateLevel(float dt) {
    physicsTick(dt);
}

void Level::updateAIs(float dt) {
    for(int i=0;i<nplanes;++i){
        vec3 p = planes[i]->pos();
        if(p.z>300)p.z-=600;
        p.z += dt*30;
        planes[0]->pos(p);
    }
    for(AI_entry en : ais) {
        en.time_left -= dt;
        //if(en.time_left <= 0) {
            en.time_left = en.tickrate; //REMARK could be better with time_left += tickrate depending on behavior
            en.ai->updateAI();
        //}
    }
}

/**
 * ONLY physics (and unimportant graphics update) behaviors
 * go in here. This function is called both (1) by the server
 * as well as (2) by the client when interpolating between
 * server messages.
 *
 */
void Level::physicsTick(float dt) {
    for(std::pair<int, Entity *> en : entities) {
        // if(en.second->velocity.y != en.second->velocity.y)en.second->velocity = vec3();
        // en.second->position += dt*en.second->velocity;
        //fprintf(stderr,"%s, %f, %f\n",en.second->getName().c_str(),dt,en.second->vel().y);
        en.second->onTick(dt);
        en.second->updatePhysicsVolume();
        //printf("Doing physics tick for entity %p\n", en.second);
    }
    handleCollisions(dt);
}

void Level::handleCollisions(float dt) {
    static map<int, bool> processed;
    for(std::pair<int, Entity *> e : entities)
        processed[e.first]=false;

    for(std::pair<int, Entity *> en1 : entities) {
        // fprintf(stderr,"col %d %d %s\n",en1.first, en1.second->getID(),en1.second->getName().c_str());
        for(std::pair<int, Entity *> en2 : entities) {
            if(processed[en2.first])continue;
            if(en1.first == en2.first)continue;
            if(!en2.second->getVolume() || !en1.second->getVolume())continue;
            // fprintf(stderr,"check %d %d\n",en1.first,en2.first);
            Entity &e1 = *(en1.second);
            Entity &e2 = *(en2.second);
            vec3 e1p = e1.pos();
            vec3 pushe1 = e1.getVolume()->push(e2.getVolume());

            // fprintf(stderr,"(%.3f,%.3f,%.3f)\n",e1p.x,e1p.y,e1p.z);
            // fprintf(stderr,"(%.3f,%.3f,%.3f)\n",pushe1.x,pushe1.y,pushe1.z);
            if(pushe1 != vec3()){
                Physics::CollisionMode cm1 = e1.onCollide(en2.second);
                Physics::CollisionMode cm2 = e2.onCollide(en1.second);

                if(cm1.hardness != 0 && cm2.hardness != 0){
                    // there is a collision, and we have to handle it.
                    float weighting = e1.mass()/(e1.mass()+e2.mass());
                    e1.pos(e1.pos()-(1-weighting)*pushe1);
                    e2.pos(e2.pos()+weighting*pushe1);

                    // vec3 v1 = (e1.vel()*(e1.mass()-e2.mass()) + e2.vel()*2.f*e2.mass())/(e1.mass()+e2.mass());
                    // vec3 v2 = (e2.vel()*(e2.mass()-e1.mass()) + e1.vel()*2.f*e1.mass())/(e1.mass()+e2.mass());

                    // normalized direction from e1 to e2.
                    vec3 vc = glm::normalize(pushe1);

                    // object velocities
                    vec3 v1 = e1.vel();
                    vec3 v2 = e2.vel();

                    // components in collision vector coordinate system.
                    float o1_c_comp = (glm::dot(vc,v1));
                    vec3  o1_c_remd = v1 - o1_c_comp*vc;

                    float o2_c_comp = (glm::dot(vc,v2));
                    vec3  o2_c_remd = v2 - o2_c_comp*vc;

                    // linear axis, e1->e2 positive direction.
                    o1_c_comp = (o1_c_comp);
                    o2_c_comp = (o2_c_comp);
                    
                    float magc1 = (o1_c_comp*(e1.mass()-e2.mass()) + o2_c_comp*2.f*e2.mass())/(e1.mass()+e2.mass());
                    float magc2 = (o2_c_comp*(e2.mass()-e1.mass()) + o1_c_comp*2.f*e1.mass())/(e1.mass()+e2.mass());

                    vec3 res1 = o1_c_remd + magc1*vc;
                    vec3 res2 = o2_c_remd + magc2*vc;

                    e1.vel(res1);
                    e2.vel(res2);
                }
            }
            vec3 np = e1.pos();
            // fprintf(stderr,"(%.3f,%.3f,%.3f)\n\n",np.x,np.y,np.z);
        }
        processed[en1.first] = true;
    }
}

/*void Level::updateEntities(float dt) {
    for(std::pair<int, Entity *> en : entities) {
        //printf("%d %p\n", en.first, en.second);
        en.second->onTick(dt);
    
    }
}*/

