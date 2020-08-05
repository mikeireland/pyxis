//=============================================================================
// Copyright (c) 2001-2019 FLIR Systems, Inc. All Rights Reserved.
//
// This software is the confidential and proprietary information of FLIR
// Integrated Imaging Solutions, Inc. ("Confidential Information"). You
// shall not disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with FLIR Integrated Imaging Solutions, Inc. (FLIR).
//
// FLIR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. FLIR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

struct Chunky{
    int64_t frameID;
    uint64_t timestamp;
    double exposureTime;
    int64_t height;
    int64_t width;
    int64_t offsetX;
    int64_t offsetY;
    double gain;
}

// This function configures the camera to add chunk data to each image. It does
// this by enabling each type of chunk data before enabling chunk data mode.
// When chunk data is turned on, the data is made available in both the nodemap
// and each image.
int ConfigureChunkData(INodeMap& nodeMap)
{
    int result = 0;
    cout << endl << endl << "*** CONFIGURING CHUNK DATA ***" << endl << endl;
    try
    {
        //
        // Activate chunk mode
        //
        // *** NOTES ***
        // Once enabled, chunk data will be available at the end of the payload
        // of every image captured until it is disabled. Chunk data can also be
        // retrieved from the nodemap.
        //
        CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");
        if (!IsAvailable(ptrChunkModeActive) || !IsWritable(ptrChunkModeActive))
        {
            cout << "Unable to activate chunk mode. Aborting..." << endl << endl;
            return -1;
        }
        ptrChunkModeActive->SetValue(true);
        cout << "Chunk mode activated..." << endl;
        //
        // Enable all types of chunk data
        //
        // *** NOTES ***
        // Enabling chunk data requires working with nodes: "ChunkSelector"
        // is an enumeration selector node and "ChunkEnable" is a boolean. It
        // requires retrieving the selector node (which is of enumeration node
        // type), selecting the entry of the chunk data to be enabled, retrieving
        // the corresponding boolean, and setting it to true.
        //
        // In this example, all chunk data is enabled, so these steps are
        // performed in a loop. Once this is complete, chunk mode still needs to
        // be activated.
        //
        NodeList_t entries;
        // Retrieve the selector node
        CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
        if (!IsAvailable(ptrChunkSelector) || !IsReadable(ptrChunkSelector))
        {
            cout << "Unable to retrieve chunk selector. Aborting..." << endl << endl;
            return -1;
        }
        // Retrieve entries
        ptrChunkSelector->GetEntries(entries);
        cout << "Enabling entries..." << endl;
        for (size_t i = 0; i < entries.size(); i++)
        {
            // Select entry to be enabled
            CEnumEntryPtr ptrChunkSelectorEntry = entries.at(i);
            // Go to next node if problem occurs
            if (!IsAvailable(ptrChunkSelectorEntry) || !IsReadable(ptrChunkSelectorEntry))
            {
                continue;
            }
            ptrChunkSelector->SetIntValue(ptrChunkSelectorEntry->GetValue());
            cout << "\t" << ptrChunkSelectorEntry->GetSymbolic() << ": ";
            // Retrieve corresponding boolean
            CBooleanPtr ptrChunkEnable = nodeMap.GetNode("ChunkEnable");
            // Enable the boolean, thus enabling the corresponding chunk data
            if (!IsAvailable(ptrChunkEnable))
            {
                cout << "not available" << endl;
                result = -1;
            }
            else if (ptrChunkEnable->GetValue())
            {
                cout << "enabled" << endl;
            }
            else if (IsWritable(ptrChunkEnable))
            {
                ptrChunkEnable->SetValue(true);
                cout << "enabled" << endl;
            }
            else
            {
                cout << "not writable" << endl;
                result = -1;
            }
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}

// This function displays a select amount of chunk data from the image. Unlike
// accessing chunk data via the nodemap, there is no way to loop through all
// available data.
int GetChunkData(ImagePtr pImage, Chunky imageChunk)
{
    int result = 0;
    try
    {
        //
        // Retrieve chunk data from image
        //
        // *** NOTES ***
        // When retrieving chunk data from an image, the data is stored in a
        // a ChunkData object and accessed with getter functions.
        //
        ChunkData chunkData = pImage->GetChunkData();
        //
        // Retrieve exposure time; exposure time recorded in microseconds
        //
        // *** NOTES ***
        // Floating point numbers are returned as a float64_t. This can safely
        // and easily be statically cast to a double.
        //
        imageChunk.exposureTime = static_cast<double>(chunkData.GetExposureTime());
        //
        // Retrieve frame ID
        //
        // *** NOTES ***
        // Integers are returned as an int64_t. As this is the typical integer
        // data type used in the Spinnaker SDK, there is no need to cast it.
        //
        imageChunk.frameID = chunkData.GetFrameID();
        // Retrieve gain; gain recorded in decibels
        imageChunk.gain = chunkData.GetGain();
        // Retrieve height; height recorded in pixels
        imageChunk.height = chunkData.GetHeight();
        // Retrieve offset X; offset X recorded in pixels
        imageChunk.offsetX = chunkData.GetOffsetX();
        // Retrieve offset Y; offset Y recorded in pixels
        imageChunk.offsetY = chunkData.GetOffsetY();
        // Retrieve timestamp
        imageChunk.timestamp = chunkData.GetTimestamp();
        // Retrieve width; width recorded in pixels
        imageChunk.width = chunkData.GetWidth();
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}

// This function disables each type of chunk data before disabling chunk data mode.
int DisableChunkData(INodeMap& nodeMap)
{
    int result = 0;
    try
    {
        NodeList_t entries;
        // Retrieve the selector node
        CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
        if (!IsAvailable(ptrChunkSelector) || !IsReadable(ptrChunkSelector))
        {
            cout << "Unable to retrieve chunk selector. Aborting..." << endl << endl;
            return -1;
        }
        // Retrieve entries
        ptrChunkSelector->GetEntries(entries);
        cout << "Disabling entries..." << endl;
        for (size_t i = 0; i < entries.size(); i++)
        {
            // Select entry to be disabled
            CEnumEntryPtr ptrChunkSelectorEntry = entries.at(i);
            // Go to next node if problem occurs
            if (!IsAvailable(ptrChunkSelectorEntry) || !IsReadable(ptrChunkSelectorEntry))
            {
                continue;
            }
            ptrChunkSelector->SetIntValue(ptrChunkSelectorEntry->GetValue());
            cout << "\t" << ptrChunkSelectorEntry->GetSymbolic() << ": ";
            // Retrieve corresponding boolean
            CBooleanPtr ptrChunkEnable = nodeMap.GetNode("ChunkEnable");
            // Disable the boolean, thus disabling the corresponding chunk data
            if (!IsAvailable(ptrChunkEnable))
            {
                cout << "not available" << endl;
                result = -1;
            }
            else if (!ptrChunkEnable->GetValue())
            {
                cout << "disabled" << endl;
            }
            else if (IsWritable(ptrChunkEnable))
            {
                ptrChunkEnable->SetValue(false);
                cout << "disabled" << endl;
            }
            else
            {
                cout << "not writable" << endl;
            }
        }
        cout << endl;
        // Deactivate ChunkMode
        CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");
        if (!IsAvailable(ptrChunkModeActive) || !IsWritable(ptrChunkModeActive))
        {
            cout << "Unable to deactivate chunk mode. Aborting..." << endl << endl;
            return -1;
        }
        ptrChunkModeActive->SetValue(false);
        cout << "Chunk mode deactivated..." << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}
