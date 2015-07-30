/*

	Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>

#include "mesh.h"

//include texture
#include "texture.h"
#include "reflectionTexture.h"
#include "color.h"
#include "err.h"
#include "Output.h"

const float DELTA_IN_FRONT_OF_CANDIDE3 = 3.0; //delta face behind the glasses

#if defined(DESKTOP_GL)
const float DELTA_BIGGER_THAN_CANDIDE3 = 1.03; //delta face width smaller than glasses
#else //DESKTOP_GL
const float DELTA_BIGGER_THAN_CANDIDE3 = -1.05; //delta face width smaller than glasses
#endif //DESKTOP_GL

Mesh::MeshEntry::MeshEntry()
{
    VB = INVALID_OGL_VALUE;
    IB = INVALID_OGL_VALUE;
    NumIndices  = 0;
    MaterialIndex = INVALID_MATERIAL;
};

Mesh::MeshEntry::~MeshEntry()
{
    if (VB != INVALID_OGL_VALUE)
    {
        glDeleteBuffers(1, &VB);
    }

    if (IB != INVALID_OGL_VALUE)
    {
        glDeleteBuffers(1, &IB);
    }
}

void Mesh::MeshEntry::Init(const std::vector<Vertex>& Vertices,
                          const std::vector<unsigned int>& Indices)
{
    NumIndices = (unsigned int)Indices.size();

    glGenBuffers(1, &VB);
  	glBindBuffer(GL_ARRAY_BUFFER, VB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NumIndices, &Indices[0], GL_STATIC_DRAW);
}

Mesh::Mesh()
{
    xMax = 0;
    yMax = 0;
    zMax = 0;
    xMin = 0;
    yMin = 0;
    zMin = 0;
}


Mesh::~Mesh()
{
    Clear();
}


void Mesh::Clear()
{
    for (unsigned int i = 0 ; i < m_Materials.size() ; i++) {
        delete (m_Materials[i]);
    }
    m_Entries.resize(0);
    m_Materials.resize(0);
}
bool Mesh::reloadMesh( const std::string& Filename, float zRotateInDegree )
{
    Clear();
    bool ret = false;
    Assimp::Importer Importer;
    
    const aiScene* pScene = Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    if (pScene) {
        ret = InitFromScene(pScene, Filename, zRotateInDegree);
    } else {
        OUTPUT("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
    }
    //fake memory leak with pScene not released?
    //http://sourceforge.net/p/assimp/discussion/817654/thread/1ef7668d/
    Importer.FreeScene();
    return ret;
}

float Mesh::getVecWidth(vector<myvec3>* vec)
{
    float xMin = 0;
    float xMax = 0;
    size_t total = vec->size();
    for (size_t i = 0; i < total ; i++){
        myvec3 vert = (*vec)[i];
        if( vert.x > xMax ) {
            xMax = vert.x;
        }
        if( vert.x < xMin ) {
            xMin = vert.x;
        }
    }
    return (xMax - xMin);
}

bool Mesh::LoadMesh(const std::string& Filename, const char*candide3FacePath, const char* candide3VertPath, float zRotateInDegree,
                    bool bUploadCandide3Vertices, vector<myvec3>* candide3Vec)
{
    // Release the previously loaded mesh (if it exists)
    Clear();
    
    bool ret = false;
    Assimp::Importer Importer;
    
    //width ratio from candide3 to glasses
    const aiScene* pScene = Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    if (pScene) {
        if( bUploadCandide3Vertices ) {
            float candide3Width = Mesh::getVecWidth(candide3Vec);
            getMeshWidthInfo(pScene, Filename);
            _candide3WidthRatio = (candide3Width * DELTA_BIGGER_THAN_CANDIDE3)/getWidth(); //glasses is a little bigger than candid3
        } else {
            _candide3WidthRatio = 1;
        }
        ret = InitFromScene(pScene, Filename, zRotateInDegree);
    } else {
        OUTPUT("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
    }
    
    ////////////////////////
    //Load candide3
    ////////////////////////
    string candide3FaceP = candide3FacePath;
    _candide3.readFaces(candide3FaceP);
    if( bUploadCandide3Vertices ) {
        _candide3.setCandide3Vertices(candide3Vec, zRotateInDegree);
    } else {
        string candide3VertP = candide3VertPath;
        _candide3.readVertices(candide3VertP, getWidth(), zRotateInDegree);
    }
    //fake memory leak with pScene not released?
    //http://sourceforge.net/p/assimp/discussion/817654/thread/1ef7668d/
    Importer.FreeScene();
    return ret;
}

bool Mesh::InitFromScene(const aiScene* pScene, const std::string& Filename, float zRotateInDegree)
{  
    m_Entries.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh, zRotateInDegree);
    }
    
#if defined(DESKTOP_GL)
    glGenVertexArrays(1, &vao);
#endif //DESKTOP_GL
    /*
    OUTPUT("Max vertex coord:%.2f, %.2f, %.2f\n",
           xMax, yMax, zMax);
    
    OUTPUT("Min vertex coord:%.2f, %.2f, %.2f\n",
           xMin, yMin, zMin);
    */
    return InitMaterials(pScene, Filename);
}

void Mesh::InitMesh(unsigned int Index, const aiMesh* paiMesh, float zRotateInDegree)
{
    m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;
    
    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;
    
    float widthRatio = _candide3WidthRatio;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    
    /*
     OUTPUT("Mesh Index=%d, Material Index='%d', vertices=%d, mNumFaces=%d\n", Index,
           paiMesh->mMaterialIndex, paiMesh->mNumVertices, paiMesh->mNumFaces);
     */

    int yFloatUp = 8;
    if( zRotateInDegree == 90 ) {
        yFloatUp = 15;
    }
    
    float deltaInFrontOfCandide3 = 0;
    if( widthRatio > 0) {
        deltaInFrontOfCandide3 = DELTA_IN_FRONT_OF_CANDIDE3;
    }
    
    for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
        const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal   = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Vertex v(Vector3f(pPos->x * widthRatio, (pPos->y+yFloatUp) * widthRatio, (pPos->z + deltaInFrontOfCandide3) * widthRatio),
                 Vector2f(pTexCoord->x, pTexCoord->y),
                 Vector3f(pNormal->x, pNormal->y, pNormal->z));
        
        //OUTPUT("Mesh vertice x=%.2f, y=%.2f, z=%.2f\r\n", v.m_pos.x, v.m_pos.y, v.m_pos.z);
        //OUTPUT("Mesh normal x=%.2f, y=%.2f, z=%.2f\r\n", pNormal->x, pNormal->y, pNormal->z);
        
        if( pPos->x * widthRatio > xMax ) {
            xMax = pPos->x * widthRatio;
        }
        if( (pPos->y+yFloatUp) * widthRatio > yMax ) {
            yMax = (pPos->y+yFloatUp)* widthRatio;
        }
        if( pPos->z * widthRatio > zMax ) {
            zMax = pPos->z * widthRatio;
        }
        
        if( pPos->x * widthRatio < xMin ) {
            xMin = pPos->x * widthRatio;
        }
        if( (pPos->y+yFloatUp) * widthRatio< yMin ) {
            yMin = (pPos->y+yFloatUp) * widthRatio;
        }
        if( pPos->z * widthRatio < zMin ) {
            zMin = pPos->z * widthRatio;
        }

        Vertices.push_back(v);
    }

    for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries[Index].Init(Vertices, Indices);
}

bool Mesh::InitMaterials(const aiScene* pScene, const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/");
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Materials[i] = NULL;
        
        aiString name;
        pMaterial->Get(AI_MATKEY_NAME,name);
        
        aiColor4D diffuse;
        aiColor4D ambient;
        aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
        aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &ambient);
        
        //All other colors are read as default values.
        aiColor4D specular;
        aiColor4D emissive;
        aiColor4D transparent;
        float shininess = 0.0;
        unsigned int max;
        aiGetMaterialFloatArray(pMaterial, AI_MATKEY_SHININESS, &shininess, &max);
        aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &specular);
        aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_EMISSIVE, &emissive);
        aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_TRANSPARENT, &transparent);
        
        Vector4f diffuseColor(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
        Vector4f ambientColor(ambient.r, ambient.g, ambient.b, ambient.a);
        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string FullPath = Dir + "/" + Path.data;
                if( !strcmp(Path.data, "ramp1-nurbsToPoly1.png") ) {
                    std::string reflectionFullPath = Dir + "/Brooklyn_Bridge_Planks_1k.hdr";
                    m_Materials[i] = new ReflectionTexture(m_texCountLocation,
                                                           m_diffuseColorLocation,
                                                           m_ambientColorLocation,
                                                           m_textureImageLocation,
                                                           diffuseColor,
                                                           ambientColor,
                                                           FullPath.c_str(),
                                                           m_envMapLocation,
                                                           reflectionFullPath.c_str()
                                                           );
                } else {
                    m_Materials[i] = new Texture(m_texCountLocation,
                                             m_diffuseColorLocation,
                                             m_ambientColorLocation,
                                             m_textureImageLocation,
                                             diffuseColor,
                                             ambientColor,
                                             FullPath.c_str());
                }

                if (!m_Materials[i]->load()) {
                    OUTPUT("Error loading texture '%s'\n", FullPath.c_str());
                    delete m_Materials[i];
                    m_Materials[i] = NULL;
                    Ret = false;
                } else {
                    /*OUTPUT("Loaded texture index:%d, name %s file: %s coord:%.2f, %.2f, %.2f, %.2f: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f, %.2f, %d\n",
                           i, name.C_Str(), Path.data,
                           diffuseColor.x, diffuseColor.y, diffuseColor.z, diffuseColor.w,
                           ambientColor.x, ambientColor.y, ambientColor.z, ambientColor.w,
                           specular.r, specular.g, specular.b, specular.a,
                           emissive.r, emissive.g, emissive.b, emissive.a,
                           transparent.r, transparent.g, transparent.b, transparent.a,
                           shininess, max);
                     */
                }
            }
        } else {
            m_Materials[i] = new Color(m_texCountLocation,
                                       m_diffuseColorLocation,
                                       m_ambientColorLocation,
                                       m_textureImageLocation,
                                       diffuseColor,
                                       ambientColor);
            /*
            OUTPUT("Loaded color index:%d, name %s coord:%.2f, %.2f, %.2f, %.2f: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f, %.2f, %d\n",
                   i, name.C_Str(),
                   diffuseColor.x, diffuseColor.y, diffuseColor.z, diffuseColor.w,
                   ambientColor.x, ambientColor.y, ambientColor.z, ambientColor.w,
                   specular.r, specular.g, specular.b, specular.a,
                   emissive.r, emissive.g, emissive.b, emissive.a,
                   transparent.r, transparent.g, transparent.b, transparent.a,
                   shininess, max);
            */
        }
    }

    return Ret;
}

void Mesh::Render(GLuint textureObj)
{
#if defined(DESKTOP_GL)
    //according to http://stackoverflow.com/questions/24643027/opengl-invalid-operation-following-glenablevertexattribarray
    //enable core profile, 3.2 only
    glBindVertexArray( vao );
#endif //DESKTOP_GL
    
    glEnableVertexAttribArray(m_positionLocation);
    glEnableVertexAttribArray(m_texCoordLocation);
    glEnableVertexAttribArray(m_normalLocation);
    
    //first render invisible candide3
#if defined(DESKTOP_GL)
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif //DESKTOP_GL
    _candide3.render(textureObj);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    
    //then render visible glasses object
    unsigned int totalMeshes = (unsigned int)m_Entries.size();
    for (unsigned int i = 0 ; i <  totalMeshes; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
        glVertexAttribPointer(m_positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); //3*4
        glVertexAttribPointer(m_texCoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12); //2*4
        glVertexAttribPointer(m_normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20); //3*4

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);
        
        /*
        //use depth-bias to make the lens away from the frame to avoid z-fighting
        const float polygonOffsetFactor = 32.0f;
        const float polygonOffsetUnits = 32.0f;
        glDepthFunc(GL_LEQUAL);
        */
        
        //starting from GL_TEXTURE1 to avoid conflict with GL_TEXTURE0 in the base texture.
        const unsigned int materialIndex = m_Entries[i].MaterialIndex;
        if( materialIndex < m_Materials.size() && m_Materials[materialIndex] ){
            /*
            if( dynamic_cast<ReflectionTexture*>(m_Materials[materialIndex]) ) {
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits);
            } else {
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
            */
            m_Materials[materialIndex]->bind(1);
        }
        glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
    }
    
    /*
    glLineWidth(2.5);
    glColor3f(1.0, 0.0, 0.0);
    
    unsigned int indices[] = {0, 1};
    
    GLuint VB;
    GLuint IB;
    std::vector<Vertex> ver;
    std::vector<unsigned int> ind;
    ver.push_back(Vertex(Vector3f(-10, 0, 50),
                         Vector2f(0, 0),
                         Vector3f(0, 0, 0)));
    ver.push_back(Vertex(Vector3f(15, -10, 6),
                         Vector2f(0, 0),
                         Vector3f(0, 0, 0)));
    ver.push_back(Vertex(Vector3f(10, -20, 70),
                         Vector2f(0, 0),
                         Vector3f(0, 0, 0)));
    ind.push_back(0);
    ind.push_back(1);
    ind.push_back(2);
    
    glGenBuffers(1, &VB);
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, &ver[0], GL_STATIC_DRAW);
    
    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3, &ind[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glVertexAttribPointer(m_positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); //3*4

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &VB);
    glDeleteBuffers(1, &IB);
    */
    
    glDisableVertexAttribArray(m_positionLocation);
    glDisableVertexAttribArray(m_texCoordLocation);
    glDisableVertexAttribArray(m_normalLocation);
    
}

int Mesh::getMeshWidthInfo(const aiScene* pScene, const std::string& Filename)
{
    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0 ; i < pScene->mNumMeshes ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        
        for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
            const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
            
            if( pPos->x > xMax ) {
                xMax = pPos->x;
            }
            if( pPos->y > yMax ) {
                yMax = pPos->y;
            }
            if( pPos->z > zMax ) {
                zMax = pPos->z;
            }
            
            if( pPos->x < xMin ) {
                xMin = pPos->x;
            }
            if( pPos->y < yMin ) {
                yMin = pPos->y;
            }
            if( pPos->z < zMin ) {
                zMin = pPos->z;
            }
        }
    }
    //OUTPUT("Glasses width:%.2f\r\n", (xMax-xMin));
    return 1;
}
