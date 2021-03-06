// Copyright (c) 2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "JSONExport.h"

using namespace rapidjson;
using namespace std::tr1;
using namespace std;

namespace JSONExport 
{
    //-- Serializers

    shared_ptr <JSONExport::JSONObject> serializeBuffer(JSONBuffer* buffer, void *context)
    {
        shared_ptr <JSONExport::JSONObject> bufferObject(new JSONExport::JSONObject());
        
        bufferObject->setString("type", "ArrayBuffer");
        bufferObject->setUnsignedInt32("byteLength", (unsigned int)buffer->getByteSize());
        bufferObject->setString("path", buffer->getID());
        
        return bufferObject;
    }
    
    shared_ptr <JSONExport::JSONObject> serializeEffect(JSONEffect* effect, void *context)
    {
        shared_ptr <JSONExport::JSONObject> effectObject(new JSONExport::JSONObject());
        shared_ptr <JSONExport::JSONArray> colorObject(new JSONExport::JSONArray());
        
        effectObject->setString("technique", effect->getTechniqueID());
        effectObject->setString("name", effect->getName());
        effectObject->setValue("techniques", effect->getTechniques());
        
        return effectObject;
    }
    
    shared_ptr <JSONExport::JSONObject> serializeMesh(JSONMesh* mesh, void *context)
    {
        typedef map<std::string , shared_ptr<JSONExport::JSONBuffer> > IDToBufferDef;
        IDToBufferDef IDToBuffer;
        
        shared_ptr <JSONExport::JSONObject> meshObject(new JSONExport::JSONObject());
        
        meshObject->setString("name", mesh->getName());
        
        //primitives
        shared_ptr <JSONExport::JSONArray> primitivesArray(new JSONExport::JSONArray());
        meshObject->setValue("primitives", primitivesArray);
        
        //accessors
        shared_ptr <JSONExport::JSONObject> accessorsObject(new JSONExport::JSONObject());
        meshObject->setValue("accessors", accessorsObject);
        
        vector <shared_ptr< JSONExport::JSONAccessor> > allAccessors = mesh->remappedAccessors();
        
        std::vector< shared_ptr<JSONExport::JSONPrimitive> > primitives = mesh->getPrimitives();
        unsigned int primitivesCount =  (unsigned int)primitives.size();
        for (unsigned int i = 0 ; i < primitivesCount ; i++) {
            
            shared_ptr<JSONExport::JSONPrimitive> primitive = primitives[i];
            shared_ptr <JSONExport::JSONIndices> uniqueIndices =  primitive->getUniqueIndices();
            
            void *primitiveContext[2];
            
            primitiveContext[0] = mesh;
            primitiveContext[1] = context;
            
            shared_ptr <JSONExport::JSONObject> primitiveObject = serializePrimitive(primitive.get(), primitiveContext);
            
            primitivesArray->appendValue(primitiveObject);
            
            //FIXME: check to remove that code
            unsigned int indicesCount = (unsigned int)primitive->getUniqueIndices()->getCount();
            if (indicesCount <= 0) {
                // FIXME: report error
            } else {
                size_t indicesLength = sizeof(unsigned short) * indicesCount;
                shared_ptr <JSONExport::JSONBuffer> indiceBuffer(new JSONExport::JSONBuffer(uniqueIndices->getBuffer()->getID(), indicesLength));
                IDToBuffer[indiceBuffer->getID()] = indiceBuffer;
            }
            
        }
        
        vector <JSONExport::Semantic> allSemantics = mesh->allSemantics();
        for (unsigned int i = 0 ; i < allSemantics.size() ; i++) {
            JSONExport::Semantic semantic = allSemantics[i];
            
            JSONExport::IndexSetToAccessorHashmap::const_iterator accessorIterator;
            JSONExport::IndexSetToAccessorHashmap& indexSetToAccessor = mesh->getAccessorsForSemantic(semantic);
            
            //FIXME: consider turn this search into a method for mesh
            for (accessorIterator = indexSetToAccessor.begin() ; accessorIterator != indexSetToAccessor.end() ; accessorIterator++) {
                //(*it).first;             // the key value (of type Key)
                //(*it).second;            // the mapped value (of type T)
                shared_ptr <JSONExport::JSONAccessor> accessor = (*accessorIterator).second;
                
                shared_ptr <JSONExport::JSONObject> accessorObject = serializeAccessor(accessor.get(), context);
                
                accessorsObject->setValue(accessor->getID(), accessorObject);
            }
        }
        
        return meshObject;
    }
    
    shared_ptr <JSONExport::JSONObject> serializeAccessor(JSONAccessor* accessor, void *context)
    {
        shared_ptr <JSONObject> accessorObject = shared_ptr<JSONObject>(new JSONObject());
        
        accessorObject->setUnsignedInt32("byteStride", (unsigned int)accessor->getByteStride());
        accessorObject->setUnsignedInt32("byteOffset", (unsigned int)accessor->getByteOffset());
        accessorObject->setUnsignedInt32("elementsPerValue", (unsigned int)accessor->getElementsPerVertexAttribute());
        accessorObject->setUnsignedInt32("count", (unsigned int)accessor->getCount());
        accessorObject->setString("elementType", JSONUtils::getStringForType(accessor->getElementType()));
        
        JSONBuffer* buffer = context ? (JSONBuffer*)context : accessor->getBuffer().get();
        
        accessorObject->setString("buffer", buffer->getID());
        
        const double* min = accessor->getMin();
        if (min) {
            shared_ptr <JSONExport::JSONArray> minArray(new JSONExport::JSONArray());
            accessorObject->setValue("min", minArray);
            for (size_t i = 0 ; i < accessor->getElementsPerVertexAttribute() ; i++) {
                minArray->appendValue(shared_ptr <JSONExport::JSONNumber> (new JSONExport::JSONNumber(min[i])));
            }
        }
        
        
        const double* max = accessor->getMax();
        if (max) {
            shared_ptr <JSONExport::JSONArray> maxArray(new JSONExport::JSONArray());
            accessorObject->setValue("max", maxArray);
            for (size_t i = 0 ; i < accessor->getElementsPerVertexAttribute() ; i++) {
                maxArray->appendValue(shared_ptr <JSONExport::JSONNumber> (new JSONExport::JSONNumber(max[i])));
            }
        }
        
        return accessorObject;
    }
    
    
    shared_ptr <JSONExport::JSONObject> serializeIndices(JSONIndices* indices, void *context)
    {
        shared_ptr <JSONExport::JSONObject> indicesObject(new JSONExport::JSONObject());
        
        JSONBuffer* buffer = context ? (JSONBuffer*)context : indices->getBuffer().get();
        
        indicesObject->setString("type", JSONUtils::getStringForTypedArray(JSONExport::UNSIGNED_SHORT));
        indicesObject->setString("buffer", buffer->getID());
        indicesObject->setUnsignedInt32("byteOffset", (unsigned int)indices->getByteOffset());
        indicesObject->setUnsignedInt32("length", (unsigned int)indices->getCount());
        
        return indicesObject;
    }
    
    shared_ptr <JSONExport::JSONObject> serializePrimitive(JSONPrimitive* primitive, void *context)
    {
        void** primitiveContext = (void**)context;
        shared_ptr <JSONExport::JSONObject> primitiveObject(new JSONExport::JSONObject());
        
        JSONMesh* mesh = (JSONMesh*)primitiveContext[0];
        
        primitiveObject->setString("primitive", primitive->getType());
        primitiveObject->setString("material", primitive->getMaterialID());
        
        shared_ptr <JSONExport::JSONArray> vertexAttributesArray(new JSONExport::JSONArray());
        primitiveObject->setValue("vertexAttributes", vertexAttributesArray);
        
        size_t count = primitive->allIndices().size();
        for (size_t j = 0 ; j < count ; j++) {
            shared_ptr <JSONExport::JSONObject> indicesObject(new JSONExport::JSONObject());
            shared_ptr <JSONExport::JSONIndices> indices = primitive->allIndices()[j];
            
            JSONExport::Semantic semantic = indices->getSemantic();
            vertexAttributesArray->appendValue(indicesObject);
            indicesObject->setString("semantic", JSONUtils::getStringForSemantic(semantic));
            unsigned int indexOfSet = 0;
            if (mesh->getAccessorsForSemantic(semantic).size() > 1) {
                indexOfSet = indices->getIndexOfSet();
                indicesObject->setString("set", JSONUtils::toString(indices->getIndexOfSet()));
            }
            indicesObject->setString("accessor", mesh->getAccessorsForSemantic(semantic)[indexOfSet]->getID());
        }
        
        shared_ptr <JSONExport::JSONIndices> uniqueIndices = primitive->getUniqueIndices();
        shared_ptr <JSONExport::JSONObject> serializedIndices = serializeIndices(uniqueIndices.get(), primitiveContext[1]);
        primitiveObject->setValue("indices", serializedIndices);
        
        return primitiveObject;
    }
    
    shared_ptr <JSONExport::JSONValue> serializeVec3(double x,double y, double z) {
        shared_ptr <JSONExport::JSONArray> vec3(new JSONExport::JSONArray());
        
        vec3->appendValue(shared_ptr <JSONExport::JSONNumber> (new JSONExport::JSONNumber(x)));
        vec3->appendValue(shared_ptr <JSONExport::JSONNumber> (new JSONExport::JSONNumber(y)));
        vec3->appendValue(shared_ptr <JSONExport::JSONNumber> (new JSONExport::JSONNumber(z)));
        
        return vec3;
    }

    //-- Writer
    
    JSONWriter::JSONWriter(rapidjson::PrettyWriter <rapidjson::FileStream> *writer):
    _writer(writer)
    {
    }
    
    JSONWriter::JSONWriter():
    _writer(0)
    {
    }
    
    JSONWriter::~JSONWriter()
    {
    }
    
    void JSONWriter::setWriter(rapidjson::PrettyWriter <rapidjson::FileStream> *writer)
    {
        this->_writer = writer;
    }
    
    rapidjson::PrettyWriter <rapidjson::FileStream>* JSONWriter::getWriter()
    {
        return this->_writer;
    }
    
    //base
    void JSONWriter::writeArray(JSONArray* array, void *context)
    {
        this->_writer->StartArray();
        
        vector <shared_ptr <JSONValue> > values = array->values();
        size_t count = values.size();
        for (size_t i = 0 ; i < count ; i++) {
            values[i]->write(this, context);
        }
        
        this->_writer->EndArray();
    }
    
    void JSONWriter::writeObject(JSONObject* object, void *context)
    {
        this->_writer->StartObject(); 

        vector <std::string> keys = object->getAllKeys();
        size_t count = keys.size();
        
        for (size_t i = 0 ; i < count ; i++) {
            shared_ptr <JSONValue> value = object->getValue(keys[i]);
            const std::string& key = keys[i];
            this->_writer->String(key.c_str());
            if (value)
                value->write(this, context);
        }
        
        this->_writer->EndObject(); 
    }
    
    void JSONWriter::writeNumber(JSONNumber* number, void *context)
    {
        JSONNumber::JSONNumberType type = number->getType();
        
        switch (type) {
            case JSONNumber::UNSIGNED_INT32:
                this->_writer->Uint(number->getUnsignedInt32());
                break;
            case JSONNumber::INT32:
                this->_writer->Int(number->getInt32());
                break;
            case JSONNumber::DOUBLE:
            {   
                double value = number->getDouble();
                this->_writer->Double(value);
                break;
            }
            case JSONNumber::BOOL:
            {   
                bool value = number->getBool();
                this->_writer->Bool(value);
            }
                break;
            default:
                //FIXME: TODO Handle error
                break;
        }
    }
        
    void JSONWriter::writeString(JSONString* str, void *context)
    {
        this->_writer->String(str->getCString());
    }
    
    void JSONWriter::write(JSONValue* value, void* context)
    {
        switch (value->getType()) {
            case JSONExport::NUMBER:
                this->writeNumber((JSONNumber*)value, context);
                break;
            case JSONExport::OBJECT:
                this->writeObject((JSONObject*)value, context);
                break;
            case JSONExport::ARRAY:
                this->writeArray((JSONArray*)value, context);
                break;
            case JSONExport::STRING:
                this->writeString((JSONString*)value, context);
                break;
            default:
                //FIXME: report error, should not reach
                break;
        }
    }
        
}
