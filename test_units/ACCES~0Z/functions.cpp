#include <strstream>
#include "functions.h"
#include "classes.h"

void  createCaseBVect(const CostNode * const costRoot, vector<CaseStruct>& caseBvect)
{
     	// 1. define a vector to hold the total cost of each sub-tree hanging from the chunk
     	vector<unsigned int> costVect(costRoot->getchild().size());
       	// 2. init costVect
       	int i = 0;
     	for(vector<CostNode*>::const_iterator iter = costRoot->getchild().begin();
     	   iter != costRoot->getchild().end(); ++iter) {
     	        size_t szBytes = 0;
     		CostNode::calcTreeSize((*iter), szBytes);
     		costVect[i] = szBytes;
     		i++;
     	}//end for

       	vector<unsigned int>::const_iterator icost = costVect.begin();
       	vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->getchunkidVectp()->begin();
       	while(icost != costVect.end() && icid != costRoot->getcMapp()->getchunkidVectp()->end()){
       		CaseStruct h = {(*icid),(*icost)};
       		caseBvect.push_back(h);
       		++icost;
       		++icid;
       	}// end while
}//createCaseBVect

void printResults(const vector<DirEntry>& resultVect, const vector<CaseStruct>& caseBvect)
{
        vector<DirEntry>::const_iterator result_iter = resultVect.begin();
        vector<CaseStruct>::const_iterator case_iter = caseBvect.begin();
        while(result_iter != resultVect.end() && case_iter != caseBvect.end()) {
                cout<<"Tree under "<<case_iter->id.getcid()<<" ==> DirEntry ("<<result_iter->bcktId.rid<<", "<<
                                        result_iter->chnkIndex<<")\n";
                result_iter++;
                case_iter++;
        }//end while
}//printResults

void printDiskBucketContents_SingleTreeAndCluster(DiskBucket* const dbuckp, unsigned int maxDepth)
{
        static int prefix = 0; // prefix of the output file name
        ostrstream file_name;
        file_name<<"OutputDiskBucket_"<<prefix<<".txt"<<ends;
        prefix++; //increase for next call

	ofstream out(file_name.str()); //create a file stream for output

        if (!out)
        {
            cerr << "creating file \"outputDiskBucket.txt\" failed\n";
        }

        out<<"****************************************************************"<<endl;
        out<<"* Contents of a DiskBucket created by                          *"<<endl;
        out<<"*         AccessManager::createTreeRegionDiskBucketInHeap      *"<<endl;
        out<<"****************************************************************"<<endl;

        out<<"\nBUCKET HEADER"<<endl;
        out<<"-----------------"<<endl;
        out<<"id: "<<dbuckp->hdr.id.rid<<endl;
        out<<"previous: "<<dbuckp->hdr.previous.rid<<endl;
        out<<"next: "<<dbuckp->hdr.next.rid<<endl;
        out<<"no_chunks: "<<dbuckp->hdr.no_chunks<<endl;
        out<<"next_offset: "<<dbuckp->hdr.next_offset<<endl;
        out<<"freespace: "<<dbuckp->hdr.freespace<<endl;
        out<<"no_subtrees: "<<int(dbuckp->hdr.no_subtrees)<<endl;
        out<<"subtree directory entries:\n";
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++)
                out<<"subtree_dir_entry["<<i<<"]: "<<dbuckp->hdr.subtree_dir_entry[i]<<endl;
        out<<"no_ovrfl_next: "<<int(dbuckp->hdr.no_ovrfl_next)<<endl;

        // for each subtree in the bucket
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++){

                out<<"\n---- SUBTREE "<<i<<" ----\n";
                //get the corresponding chunk slot range
                unsigned int treeBegin = dbuckp->hdr.subtree_dir_entry[i];
                unsigned int treeEnd;
                if(i+1==dbuckp->hdr.no_subtrees) //if this is the last subtree
                        treeEnd  = dbuckp->hdr.no_chunks; //get slot one-passed the last chunk slot of the tree
                else
                        treeEnd  = dbuckp->hdr.subtree_dir_entry[i+1]; //get the next tree's 1st slot

                //for each chunk in the range of slots that correspond to this subtree
                for(int chnk_slot = treeBegin; chnk_slot < treeEnd; chnk_slot++) {
                        //access each chunk:
                        //get a byte pointer at the beginning of the chunk
                        char* beginChunkp = dbuckp->body + dbuckp->offsetInBucket[-chnk_slot-1];

                        //first read the chunk header in order to find whether it is a DiskDirChunk, or
                        //a DiskDataChunk
                        DiskChunkHeader* chnk_hdrp = reinterpret_cast<DiskChunkHeader*>(beginChunkp);
                        if(chnk_hdrp->depth == maxDepth)//then this is a DiskDataChunk
                                printDiskDataChunk(out, beginChunkp);
                        else if (chnk_hdrp->depth < maxDepth && chnk_hdrp->depth > Chunk::MIN_DEPTH)//it is a DiskDirchunk
                                printDiskDirChunk(out, beginChunkp);
                        else {// Invalid depth!
                                if(chnk_hdrp->depth == Chunk::MIN_DEPTH)
                                        throw "printDiskBucketContents_SingleTreeAndCluster ==> depth corresponding to root chunk!\n";
                                ostrstream msg_stream;
                                msg_stream<<"printDiskBucketContents_SingleTreeAndCluster ==> chunk depth at slot "<<chnk_slot<<
                                                " inside DiskBucket "<<dbuckp->hdr.id.rid<<" is invalid\n"<<endl;
                                throw msg_stream.str();
                        }//end else
                }//end for
        }//end for
}//printDiskBucketContents_SingleTreeAndCluster


void printDiskDirChunk(ofstream& out, char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDirChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDirChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDirChunk are printed.
{
        //get a pointer to the dir chunk
        DiskDirChunk* const chnkp = reinterpret_cast<DiskDirChunk*>(startp);

        //update pointer members
        updateDiskDirChunkPointerMembers(*chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                }//end for
        }//end for
        out<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print the dir entries
        out<<"\nDiskDirChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->hdr.no_entries; i++){
                out<<"Dir entry "<<i<<": ";
                out<<chnkp->entry[i].bucketid.rid<<", "<<chnkp->entry[i].chunk_slot<<endl;;
        }//end for
}//printDiskDirChunk

void printDiskDataChunk(ofstream& out, char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDataChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDataChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDataChunk are printed.
{
        //get a pointer to the data chunk
        DiskDataChunk* const chnkp = reinterpret_cast<DiskDataChunk*>(startp);

        //update pointer members
        updateDiskDataChunkPointerMembers(*chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                }//end for
        }//end for
        out<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print no of ace
        out<<"No of ace: "<<chnkp->no_ace<<endl;

        //print the bitmap
        out<<"\nBITMAP:\n\t";
        for(int b=0; b<chnkp->hdr.no_entries; b++){
                  (!chnkp->testbit(b)) ? out<<"0" : out<<"1";
        }//end for

        //print the data entries
        out<<"\nDiskDataChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->no_ace; i++){
                out<<"Data entry "<<i<<": ";
                for(int j=0; j<chnkp->hdr.no_measures; j++){
                        out<<chnkp->entry[i].measures[j]<<", ";
                }//end for
                out<<endl;
        }//end for
}//end printDiskDataChunk

void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk)
//precondition:
//      chnk is a DiskDirChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.entry.
{

        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);

        //update chunk_id pointer
        bytep += sizeof(DiskDirChunk); // move at the end of the static part
        chnk.hdr.chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(bytep);

        //update each domain pointer
        //We need to initialize the "ordercodes" pointer inside each Domain_t:
        //place byte pointer at the 1st ordercode of the first Domain_t
        bytep += sizeof(DiskChunkHeader::Domain_t) * chnk.hdr.depth;
        //for each Domain_t
        for(int i=0; i<chnk.hdr.depth; i++){
                //place byte pointer at the 1st ordercode of the current Domain_t
                (chnk.hdr.chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(bytep);
                bytep += sizeof(DiskChunkHeader::ordercode_t) * chnk.hdr.no_dims;
        }//end for

        //update oc_range pointer
        // in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        chnk.hdr.oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(bytep);

        //update entry pointer
        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        chnk.entry = reinterpret_cast<DiskDirChunk::DirEntry_t*>(bytep);
}//updateDiskDirChunkPointerMembers


void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk)
//precondition:
//      chnk is a DiskDataChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.bitmap, chnk.entry, chnk.entry[i].measures (0<= i <chnk.no_ace).
{

        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);

        //update chunk_id pointer
        bytep += sizeof(DiskDataChunk); // move at the end of the static part
        chnk.hdr.chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(bytep);

        //update each domain pointer
        //We need to initialize the "ordercodes" pointer inside each Domain_t:
        //place byte pointer at the 1st ordercode of the first Domain_t
        bytep += sizeof(DiskChunkHeader::Domain_t) * chnk.hdr.depth;
        //for each Domain_t
        for(int i=0; i<chnk.hdr.depth; i++){
                //place byte pointer at the 1st ordercode of the current Domain_t
                (chnk.hdr.chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(bytep);
                bytep += sizeof(DiskChunkHeader::ordercode_t) * chnk.hdr.no_dims;
        }//end for

        //update oc_range pointer
        // in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        chnk.hdr.oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(bytep);

        //update bitmap pointer
        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        chnk.bitmap = reinterpret_cast<WORD*>(bytep);

        //update entry pointer
        bytep += sizeof(WORD)* ::numOfWords(chnk.hdr.no_entries); // move to the 1st data entry
        chnk.entry = reinterpret_cast<DiskDataChunk::DataEntry_t*>(bytep);

        //move byte pointer at the first measure value
        bytep += sizeof(DiskDataChunk::DataEntry_t)*chnk.no_ace;
        //for each data entry update measures pointer
        for(int e=0; e<chnk.no_ace; e++){
                //place measures pointer at the first measure
                chnk.entry[e].measures = reinterpret_cast<measure_t*>(bytep);
                bytep += sizeof(measure_t)*chnk.hdr.no_measures; //move on to next set of measures
        }//end for
}//updateDiskDataChunkPointerMembers


void createSingleTree(CostNode* &root, unsigned int maxDepth, unsigned int numFacts, unsigned int numDims)
{

        //create root node 0|0

        //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0"));
        //      depth
        hdrp->depth = 1;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m;
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("country"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("type"),0,1));


        //create cell map
        CellMap* clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0"));
        clmpp->insert(string("0|0.0|1"));
        clmpp->insert(string("0|0.1|0"));
        clmpp->insert(string("0|0.1|1"));

        root = new CostNode(hdrp, clmpp);

        //create the children nodes

        // create node 0|0.0|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.0|-1"));
        clmpp->insert(string("0|0.0|0.1|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.0|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 1;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"), 0,1));


        //create cell map
        clmpp = new CellMap;
        //clmpp->insert(string("0|0.0|0.0|-1.0|0"));
        clmpp->insert(string("0|0.0|0.0|-1.0|1"));

        // add node to its father
        CostNode* father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.1|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),1,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"), 0,1));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.1|-1.1|0"));
        clmpp->insert(string("0|0.0|0.1|-1.1|1"));
        clmpp->insert(string("0|0.0|0.1|-1.2|0"));
        clmpp->insert(string("0|0.0|0.1|-1.2|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1"));
        clmpp->insert(string("0|0.0|1.1|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.0|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1.0|2"));
        clmpp->insert(string("0|0.0|1.0|-1.0|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.1|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),1,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.1|-1.1|2"));
        clmpp->insert(string("0|0.0|1.1|-1.1|3"));
        clmpp->insert(string("0|0.0|1.1|-1.2|2"));
        clmpp->insert(string("0|0.0|1.1|-1.2|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1"));
        clmpp->insert(string("0|0.1|0.3|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|0.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.2|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1.3|0"));
        clmpp->insert(string("0|0.1|0.2|-1.3|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.3|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),4,5));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.3|-1.4|0"));
        clmpp->insert(string("0|0.1|0.3|-1.4|1"));
        clmpp->insert(string("0|0.1|0.3|-1.5|0"));
        clmpp->insert(string("0|0.1|0.3|-1.5|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1"));
        clmpp->insert(string("0|0.1|1.3|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|1.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.2|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1.3|2"));
        clmpp->insert(string("0|0.1|1.2|-1.3|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.3|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
       							  hdrp->rlNumCells,
       							  numFacts);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),4,5));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.3|-1.4|2"));
        //clmpp->insert(string("0|0.1|1.3|-1.4|3"));
        clmpp->insert(string("0|0.1|1.3|-1.5|2"));
        //clmpp->insert(string("0|0.1|1.3|-1.5|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

}//createSingleTree

void descendBreadth1stCostTree(
				 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 queue<CostNode*>& nextToVisit,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp)
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize

// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a breadth first manner and we store each node (Chunk) as we first visit it.
{
        //ASSERTION1: non-empty sub-tree
	if(!costRoot) {
	 	//empty sub-tree
	 	throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION1: Emtpy sub-tree!!\n";
	}

	//case I: this corresponds to a directory chunk
	if(costRoot->getchunkHdrp()->depth < maxDepth){
                //Target: create the dir chunk corresponding to the current node

                //allocate entry vector where size = total no of cells
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);

                //for each child of this node
                int childAscNo = 0;
                //The current chunk will be stored at chunk-slot = dirVectp->size()
                //The current chunk's children will be stored at
                //chunk slot = current node's chunk slot + size of queue +ascending
                //number of child (starting from 1)
                int indexBase = dirVectp->size() + nextToVisit.size();
                for(vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
                    ichild != costRoot->getchild().end(); ichild++) {
                        childAscNo++; //one more child
                        //create corresponding entry in current chunk
                        DirEntry e;
			e.bcktId = bcktID; //store the bucket id where the child chunk will be stored
                        //store the chunk slot in the bucket where the chunk will be stored
                        e.chnkIndex = indexBase + childAscNo;

			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*ichild)->getchunkHdrp()->id.extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= entryVect.size())
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION2: entryVect out of range!\n";
                        //store  entry in the entry vector
			entryVect[offs] = e;

                        //push child node pointer into queue in order to visit it later
                        nextToVisit.push(*ichild);
                }//end for

                //create dir chunk instance
		DirChunk newChunk(*costRoot->getchunkHdrp(), entryVect);

	     	// store new dir chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

                //ASSERTION3: assert that queue is not empty at this point
                if(nextToVisit.empty())
                        throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION3: queue cannot be empty at this point!\n";
                //pop next node from the queue and visit it
                CostNode* next = nextToVisit.front();
                nextToVisit.pop(); // remove pointer from queue
               	try {
               		descendBreadth1stCostTree(
               				maxDepth,
               				numFacts,
               		                next,
               				factFile,
               				bcktID,
               				nextToVisit,
               				dirVectp,
               				dataVectp);
               	}
               	catch(const char* message) {
               		string msg("");
               		msg += message;
               		throw msg.c_str();
               	}
               	//ASSERTION4
               	if(dirVectp->empty())
                        throw "AccessManager::descendBreadth1stCostTree  ==>ASSERTION4: empty chunk vectors (possibly both)!!\n";
        }//end if
	else{	//case II: this corresponds to a data chunk
                //Target: create the data chunk corresponding to the current node

		//ASSERTION5: depth check
		if(costRoot->getchunkHdrp()->depth != maxDepth)
			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION5: Wrong depth in leaf chunk!\n";

               // allocate entry vector where size = real number of cells
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
               // allocate compression bitmap where size = total number of cells (initialized to 0's).
	     	bit_vector cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);

	     	// Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw "AccessManager::descendDepth1stCostTree ==> Error in creating ifstream obj\n";

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");

        	// read on until you find prefix corresponding to current data chunk
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw "AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n";
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = numFacts;
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries
		int numCellsRead = 0;
	        do {
	        // loop invariant: read a line from fact file, containing the values of
		//     		   a single cell. All cells belong to chunk with id prefix
			for(int i = 0; i<factsPerCell; ++i){
				input >> factv[i];
			}

			DataEntry e(factsPerCell, factv);
			//Offset in chunk can't be computed until bitmap is created. Store
			// the entry temporarily in this map container, by chunkid
			ChunkID cellid(buffer);

			//ASSERTION6: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION6: double entry for cell in fact load file\n";
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION7
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= cmprBmp.size()){
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION7: cmprBmp out of range!\n";
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION8: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION8: Wrong number of non-empty cells\n";

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             *costRoot->getchunkHdrp(), emptyCell);
        		//ASSERTION9: offset within range
       			if(offs >= entryVect.size())
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION9: (DataChunk) entryVect out of range!\n";

       			//ASSERTION10: non-empty cell
       			if(emptyCell)
               			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION10: access to empty cell!\n";
        		//store entry in vector
        		entryVect[offs] = map_i->second;
		}//end for

	     	// Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*costRoot->getchunkHdrp(), entryVect, cmprBmp);

	     	// Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);

                //if the queue is not empty at this point
                if(!nextToVisit.empty()){
                        //pop next node from the queue and visit it
                        CostNode* next = nextToVisit.front();
                        nextToVisit.pop(); // remove pointer from queue
                	try {
                		descendBreadth1stCostTree(
                				maxDepth,
                				numFacts,
                		                next,
                				factFile,
                				bcktID,
                				nextToVisit,
                				dirVectp,
                				dataVectp);
                	}
                	catch(const char* message) {
                		string msg("");
                		msg += message;
                		throw msg.c_str();
                	}
                	//ASSERTION11
                	if(dirVectp->empty() || dataVectp->empty())
                                throw "AccessManager::descendBreadth1stCostTree  ==>ASSERTION11: empty chunk vectors (possibly both)!!\n";
                }//end if
        }//end else
}//AccessManager::descendBreadth1stCostTree()


void descendDepth1stCostTree(
				 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp)
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize

// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a depth first manner and we store each node (Chunk) as we first visit it.
{
	if(!costRoot) {
	 	//empty sub-tree
	 	throw "AccessManager::descendDepth1stCostTree  ==> Emtpy sub-tree!!\n";
	}
	// I. If this is not a leaf chunk
	if(costRoot->getchunkHdrp()->depth < maxDepth){
	     	// 1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);

	     	// 2. Fill in those entries
	     	//    the current chunk (costRoot) will be stored in
	     	//    chunk slot == dirVectp->size()+dataVectp->size()
	     	//   the 1st child will be stored just after the current chunk
	     	unsigned int index = dirVectp->size() + dataVectp->size() + 1;
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->getchunkidVectp()->begin();
		vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
		// ASSERTION1: size of CostNode vectors is equal
		if(costRoot->getchild().size() != costRoot->getcMapp()->getchunkidVectp()->size())
			throw "AccessManager::descendDepth1stCostTree ==> ASSERTION1: != vector sizes\n";
		while(ichild != costRoot->getchild().end() && icid != costRoot->getcMapp()->getchunkidVectp()->end()){
			// loop invariant: for current entry e holds:
			//		  e.bcktId == the same for all entries
			//		  e.chnkIndex == the next available chunk slot in the bucket
			//			after all the chunks of the sub-trees hanging from
			//			the previously inserted entries.
			DirEntry e;
			e.bcktId = bcktID;
                        e.chnkIndex = index;
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*icid).extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= entryVect.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION2: entryVect out of range!\n";
       			}
			entryVect[offs] = e;
                        unsigned int numChunks = 0;
                	CostNode::countChunksOfTree(*ichild, maxDepth, numChunks);
			index = index + numChunks;
			ichild++;
			icid++;
		} //end while

	     	// 3. Now that entryVect is filled, create dirchunk object
		DirChunk newChunk(*costRoot->getchunkHdrp(), entryVect);

	     	// 4. Store new chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

	     	// 5. Descend to children
	     	for(vector<CostNode*>::const_iterator ichd = costRoot->getchild().begin();
	     	    ichd != costRoot->getchild().end(); ++ichd) {
                	try {
                		descendDepth1stCostTree(
                				maxDepth,
                				numFacts,
                				(*ichd),
                				factFile,
                				bcktID,
                				dirVectp,
                				dataVectp);
                	}
                      	catch(const char* message) {
                      		string msg("");
                      		msg += message;
                      		throw msg.c_str();
                      	}
                      	//ASSERTION3
                       	if(dirVectp->empty() || dataVectp->empty()){
                       			throw "AccessManager::descendDepth1stCostTree  ==>ASSERTION3: empty chunk vectors (possibly both)!!\n";
                       	}
	     	} // end for
	} //end if
	else { // II. this is a leaf chunk
		//ASSERTION4: depth check
		if(costRoot->getchunkHdrp()->depth != maxDepth)
			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION4: Wrong depth in leaf chunk!\n";

		// 1. Create the vector holding the chunk's entries and the compression bitmap
		//    (initialized to 0's).
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
	     	bit_vector cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);

	     	// 2. Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw "AccessManager::descendDepth1stCostTree ==> Error in creating ifstream obj\n";

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");

        	// read on until you find prefix corresponding to current data chunk
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw "AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n";
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = numFacts;
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries
		int numCellsRead = 0;
	        do {
	        // loop invariant: read a line from fact file, containing the values of
		//     		   a single cell. All cells belong to chunk with id prefix
			for(int i = 0; i<factsPerCell; ++i){
				input >> factv[i];
			}

			DataEntry e(factsPerCell, factv);
			//Offset in chunk can't be computed until bitmap is created. Store
			// the entry temporarily in this map container, by chunkid
			ChunkID cellid(buffer);

			//ASSERTION5: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw "AccessManager::descendDepth1stCostTree ==>ASSERTION5: double entry for cell in fact load file\n";
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION6
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= cmprBmp.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION6: cmprBmp out of range!\n";
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION7: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION7: Wrong number of non-empty cells\n";

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             *costRoot->getchunkHdrp(), emptyCell);
        		//ASSERTION8: offset within range
       			if(offs >= entryVect.size())
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION8: (DataChunk) entryVect out of range!\n";

       			//ASSERTION9: non-empty cell
       			if(emptyCell)
               			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION9: access to empty cell!\n";
        		//store entry in vector
        		entryVect[offs] = map_i->second;
		}//end for

	     	// 3. Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*costRoot->getchunkHdrp(), entryVect, cmprBmp);

	     	// 4. Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);
	} // end else
}// end of AccessManager::descendDepth1stCostTree

void _storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::descendBreadth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      breadth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        // for each dir chunk of this subtree
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
            dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 1: there is free space to store this dir chunk
		if(dir_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n";

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);

      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}

         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);
		}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch
                        if(dir_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n";
         	#endif

                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator
     		dbuckp->hdr.freespace -= chnk_size;

     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif

                delete chnkp; //free up memory
        }//end for

        // for each data chunk of this subtree
        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
            data_i != dataVectp->end(); data_i++){
		// loop invariant: a DataChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 2: there is free space to store this data chunk
		if(data_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n";

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);

      		// convert DataChunk to DiskDataChunk
      		DiskDataChunk* chnkp = 0;
      		try{
                        chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}

         	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{
                        placeDiskDataChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);
		}
         	catch(const char* message) {
         		string msg("");
         		msg += message;
         		throw msg.c_str();
         	}		
         	#ifdef DEBUGGING
                        //ASSERTION 2.1 : no chunk size mismatch         	
                        if(data_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1.2: DataChunk size mismatch!\n";		
         	#endif

                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;        		
     		//update free space indicator
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter        		
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory
        }//end for
}//end of AccessManager::_storeBreadth1stInDiskBucket

void _storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::descendDepth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      depth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{

        //for each dirchunk in the vector, store chunks in the row until
        // you store a (max depth-1) dir chunk. Then you have to continue selecting chunks from the
        // data chunk vector, in order to get the corresponding data chunks (under the same father).
        // After that we continue from the dir chunk vector at the point we were left.
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
                                                dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket. If this DirChunk has a maxDepth-1 depth
		//                 then we also store all the DataChunks that are its children.

		//ASSERTION 1: there is free space to store this dir chunk
		if(dir_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n";		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}		
	      	
         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);							
		}
         	catch(const char* message) {
         		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}	
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch         	
                        if(dir_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n";		
         	#endif
         		
                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator        		
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory

                //if this is a dir chunk at max depth
                if((*dir_i).gethdr().depth ==  maxDepth-1){
                        //then we have to continue selecting chunks from the DataChunk vector

                        //find all data chunks in data chunk vect with a prefix in their chunk id
                        //equal with *dir_i.gethdr().id
                        //for each datachunk in the vector
                        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
                            data_i != dataVectp->end(); data_i++){
                                //if we have a prefix match
	                        if( (*data_i).gethdr().id.getcid().find((*dir_i).gethdr().id.getcid())
	                                 != string::npos ) {

                         		// loop invariant: a DataChunk is stored in each iteration in the body
                         		//		   of the DiskBucket. All these DataChunks have the same
                         		//                 parent DirChunk

                         		//ASSERTION 2: there is free space to store this data chunk
                         		if(data_i->gethdr().size > dbuckp->hdr.freespace)
                         			throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n";		

                               		// update bucket directory (chunk slots begin from slot 0)
                               		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
                               		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
                                 		
                               		// convert DataChunk to DiskDataChunk
                               		DiskDataChunk* chnkp = 0;
                               		try{
                                                 chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);	      		
                         	      	}
                                  	catch(const char* message) {
                                  		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
                                  		msg += message;
                                  		throw msg.c_str();
                                  	}		
                         	      	
                                  	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
                               		size_t chnk_size = 0;
                               		size_t hdr_size = 0;
                               		try{      		
                                                 placeDiskDataChunkInBcktBody(chnkp, maxDepth,
                                                        nextFreeBytep, hdr_size, chnk_size);					
                         		}
                                  	catch(const char* message) {
                                  		string msg("");
                                  		msg += message;
                                  		throw msg.c_str();
                                  	}		
                                	#ifdef DEBUGGING
                                               //ASSERTION 2.1 : no chunk size mismatch         	
                                               if(data_i->gethdr().size != chnk_size)
                                                       throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 2.1: DataChunk size mismatch!\n";		
                                	#endif

                                         //update next free byte offset indicator
                              		dbuckp->hdr.next_offset += chnk_size;        		
                              		//update free space indicator
                              		dbuckp->hdr.freespace -= chnk_size;
                              		
                              		// update bucket header: chunk counter        		
                                       	dbuckp->hdr.no_chunks++;

                                       	#ifdef DEBUGGING                 	
                                       	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
                                       	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
                                       	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
                                       	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
                                       	//cout<<"current offset = "<<curr_offs<<endl;
                                        cout<<"---------------------\n";
                                        #endif								

                                        delete chnkp; //free up memory
                                }//end if
                        }//end for
                }//end if
        }//end for
}//end of AccessManager::_storeDepth1stInDiskBucket

DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth)
// precondition:
//	dirchnk is a DirChunk instance filled with valid entries. maxDepth is the maximum depth of
//      the cube in question.
// postcondition:
//	A DiskDirChunk has been allocated in heap space that contains the values of
//	dirchnk and a pointer to it is returned.	
{
	//ASSERTION 1.0 : depth and chunk-id compatibility
	if(dirchnk.gethdr().depth != dirchnk.gethdr().id.getChunkDepth())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.0: depth and chunk-id mismatch\n";	
        //ASSERTION 1.1: this is not the root chunk
        if(dirchnk.gethdr().depth == Chunk::MIN_DEPTH)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.1: this method cannot handle the root chunk\n";	
        //ASSERTION 1.2: this is not a data chunk
        if(dirchnk.gethdr().depth == maxDepth)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.2: this method cannot handle a data chunk\n";			
        //ASSERTION 1.3: valid depth value
        if(dirchnk.gethdr().depth < Chunk::MIN_DEPTH || dirchnk.gethdr().depth > maxDepth)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.3: invalid depth value\n";	
		
	//allocate new DiskDirChunk
        DiskDirChunk* chnkp=0;
	try{
		chnkp = new DiskDirChunk;
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for new DiskDirChunk!\n";		
	}	
		
	// 1. first copy the headers
	
	//insert values for non-pointer members
	chnkp->hdr.depth = dirchnk.gethdr().depth;
	chnkp->hdr.no_dims = dirchnk.gethdr().numDim;		
	chnkp->hdr.no_measures = 0; // this is a directory chunk
	chnkp->hdr.no_entries = dirchnk.gethdr().totNumCells;
	
	// store the chunk id
	//allocate space for the domains	
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[chnkp->hdr.depth];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the domains!\n";		
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION2: invalid num of dims\n";	
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != dirchnk.gethdr().id.getChunkNumOfDim(isroot))
                throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n";	
        if(isroot)
                throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 2.1: root chunk encountered\n";	
		
	//allocate space for each domain's order-codes		
	for(int i = 0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(bad_alloc){
         		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the ordercodes!\n";		
         	}			
	}//end for
	
	string cid = dirchnk.gethdr().id.getcid();	
	string::size_type begin = 0;
	for(int i=0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
		
                string::size_type b = 0;		
		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
			string::size_type e = domain.find("|", b); // get next "|"
			string ocstr = (e == string::npos) ?
			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
			b = e+1;
		}//end for
		begin = end+1;			
	}//end for
	
	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the oc ranges!\n";		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = dirchnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != dirchnk.gethdr().vectRange.size())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION3: wrong length in vector\n";	
	while(i<chnkp->hdr.no_dims && iter != dirchnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while	
	
	// 2. Next copy the entries
	//ASSERTION 3.1: valid total number of cells
	if(dirchnk.gethdr().totNumCells <= 0)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 3.1: total No of cells is <= 0\n";			
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDirChunk::DirEntry_t[dirchnk.gethdr().totNumCells];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for dir entries!\n";		
	}	
	
	i = 0;
	vector<DirEntry>::const_iterator ent_iter = dirchnk.getentry().begin();
	//ASSERTION4: combatible vector length and no of entries
	if(chnkp->hdr.no_entries != dirchnk.getentry().size())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION4: wrong length in vector\n";	
	while(i<chnkp->hdr.no_entries && ent_iter != dirchnk.getentry().end()){
	        chnkp->entry[i].bucketid = ent_iter->bcktId;
       	        chnkp->entry[i].chunk_slot = ent_iter->chnkIndex;
		i++;
		ent_iter++;	
	}//end while
	
	return chnkp;
}// end of AccessManager::dirChunk2DiskDirChunk

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
						      unsigned int maxDepth)
// precondition:
//	datachnk is a DataChunk instance filled with valid entries. numFacts is the number of facts per cell
//      and maxDepth is the maximum chunking depth of the cube in question.
// postcondition:
//	A DiskDataChunk has been allocated in heap space that contains the values of
//	datachnk and a pointer to it is returned.	
{
	//ASSERTION1: input chunk is a data chunk
	if(datachnk.gethdr().depth != maxDepth)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION1: wrong depth for data chunk\n";
		
	//allocate new DiskDataChunk
	DiskDataChunk* chnkp = 0;
	try{
		chnkp = new DiskDataChunk;
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for new DiskDataChunk!\n";		
	}	
		
	// 1. first copy the header
	
	//insert values for non-pointer members
	chnkp->hdr.depth = datachnk.gethdr().depth;
	chnkp->hdr.no_dims = datachnk.gethdr().numDim;		
	chnkp->hdr.no_measures = numFacts;
	chnkp->hdr.no_entries = datachnk.gethdr().totNumCells;
	
	// store the chunk id
	//ASSERTION 1.1 : depth and chunk-id compatibility
	if(datachnk.gethdr().depth != datachnk.gethdr().id.getChunkDepth())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 1.1: depth and chunk-id mismatch\n";		
	//allocate space for the domains	
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[chnkp->hdr.depth];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the domains!\n";		
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION2: invalid num of dims\n";	
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != datachnk.gethdr().id.getChunkNumOfDim(isroot))
                throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n";	
        if(isroot)
                throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: root chunk encountered\n";	
	//allocate space for each domain's order-codes		
	for(int i = 0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(bad_alloc){
         		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the ordercodes!\n";		
         	}			
	}//end for
	
	string cid = datachnk.gethdr().id.getcid();	
	string::size_type begin = 0;
	for(int i=0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
		
                string::size_type b = 0;		
		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
			string::size_type e = domain.find("|", b); // get next "|"
			string ocstr = (e == string::npos) ?
			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
			b = e+1;
		}//end for
		begin = end+1;			
	}//end for

	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the oc ranges!\n";		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = datachnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != datachnk.gethdr().vectRange.size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION3: wrong length in vector\n";	
	while(i<chnkp->hdr.no_dims && iter != datachnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while			
	
	// 2. Copy number of ace
	//ASSERTION 3.1: valid real and total number of cells
	if(datachnk.gethdr().totNumCells < datachnk.gethdr().rlNumCells)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 3.1: total No of cells is less than real No of cells!\n";		
	chnkp->no_ace = datachnk.gethdr().rlNumCells;
	
	// 3. Next copy the bitmap
	//ASSERTION 4: combatible bitmap length and no of entries
	if(chnkp->hdr.no_entries != datachnk.getcomprBmp().size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 4: bitmap size and No of entries mismatch\n";	
	
	//allocate space for the bitmap
	try{
		//chnkp->bitmap = new WORD[::numOfwords(datachnk.gethdr().totNumCells)];
		chnkp->allocBmp(::numOfWords(chnkp->hdr.no_entries));
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the bitmap!\n";		
	}	
	//bitmap initialization
        int bit = 0;
        for(bit_vector::const_iterator iter = datachnk.getcomprBmp().begin();
            iter != datachnk.getcomprBmp().end(); iter++){
                (*iter == true) ? chnkp->setbit(bit):chnkp->clearbit(bit);
                bit++;
        }
							
	// 4. Next copy the entries
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDataChunk::DataEntry_t[chnkp->no_ace];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n";		
	}	
	
	i = 0;
	vector<DataEntry>::const_iterator ent_iter = datachnk.getentry().begin();
	//ASSERTION5: combatible vector length and no of entries
	if(chnkp->no_ace != datachnk.getentry().size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION4: wrong length in vector\n";	
	while(i<chnkp->no_ace && ent_iter != datachnk.getentry().end()){
		//allocate space for the measures
         	try{
         		chnkp->entry[i].measures = new measure_t[int(chnkp->hdr.no_measures)];
         	}
         	catch(bad_alloc){
         		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n";
         	}	
		
         	//store the measures of this entry				
       		//ASSERTION6: combatible vector length and no of measures
		if(chnkp->hdr.no_measures != ent_iter->fact.size())
			throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION6: wrong length in vector\n";	         	
		int j=0;
		vector<measure_t>::const_iterator m_iter = ent_iter->fact.begin();
		while(j<chnkp->hdr.no_measures && m_iter != ent_iter->fact.end()){
			chnkp->entry[i].measures[j] = *m_iter;
			j++;
			m_iter++;		
		}//end while
		i++;
		ent_iter++;
	}//end while
	
	return chnkp;
}// end of AccessManager::dataChunk2DiskDataChunk

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
			char* &currentp,size_t& hdr_size, size_t& chnk_size)
// precondition:
//		chnkp points at a DiskDataChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDataChunk must be placed. maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
// postcondition:
//		the DiskDataChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDataChunk &&
//              hdr_size contains the bytes consumed by the placement of the DiskChunkHeader.
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION1: null pointer\n";

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: this is a data chunk
	if(hdrp->depth != maxDepth)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION 1.1: chunk's depth != maxDepth in Data Chunk\n";	
		
	//begin by placing the static part of a DiskDatachunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDataChunk));
	currentp += sizeof(DiskDataChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDataChunk); // this is the size of the static part of a DiskDataChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION2: null pointer\n";	
	//first store the domains of the chunk id
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
	//loop invariant: a domain of the chunk id will be stored. A domain
	// is only a pointer to an array of order codes.
		DiskChunkHeader::Domain_t* dmnp = &(hdrp->chunk_id)[i];
		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
		hdr_size += sizeof(DiskChunkHeader::Domain_t);
		chnk_size += sizeof(DiskChunkHeader::Domain_t);
	}//end for		
	//Next store the order-codes of the domains
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
		//ASSERTION3: ordercodes pointer is not null
		if(!(hdrp->chunk_id)[i].ordercodes)
			throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION3: null pointer\n";			
		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
		//loop invariant: each ordercode of the current domain will stored
			DiskChunkHeader::ordercode_t* op = &((hdrp->chunk_id)[i].ordercodes[j]);
        		memcpy(currentp, reinterpret_cast<char*>(op), sizeof(DiskChunkHeader::ordercode_t));
        		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
        		chnk_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
		
	//next place the orcercode ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION4: null pointer\n";		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
	
	//next place the bitmap (i.e., array of WORDS)
  	for(int b=0; b<numOfWords(hdrp->no_entries); b++){
		WORD* wp = &chnkp->bitmap[b];
		memcpy(currentp, reinterpret_cast<char*>(wp), sizeof(WORD));
	       	currentp += sizeof(WORD); // move on to the next empty position
       		chnk_size += sizeof(WORD);       		
  	}//end for
  	
       	//Now, place the DataEntry_t structures
       	for(int i=0; i<chnkp->no_ace; i++){
        	DiskDataChunk::DataEntry_t* ep = &chnkp->entry[i];
             	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDataChunk::DataEntry_t));
             	currentp += sizeof(DiskDataChunk::DataEntry_t); // move on to the next empty position
             	chnk_size += sizeof(DiskDataChunk::DataEntry_t);		       	
       	}//end for

       	//Finally place the measure values
       	// for each data entry
       	for(int i=0; i<chnkp->no_ace; i++){
       		// for each measure of this entry
               	for(int m=0; m<hdrp->no_measures; m++){
                	measure_t* mp = &chnkp->entry[i].measures[m];
                     	memcpy(currentp, reinterpret_cast<char*>(mp), sizeof(measure_t));
                     	currentp += sizeof(measure_t); // move on to the next empty position
                     	chnk_size += sizeof(measure_t);		       	
                }//end for
       	}//end for       	       	
}// end of AccessManager::placeDiskDataChunkInBcktBody      		

void placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
					char* &currentp, size_t& hdr_size, size_t& chnk_size)
// precondition:
//		chnkp points at a DiskDirChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDirChunk must be placed.maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
// postcondition:
//		the DiskDirChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDirChunk &&
//              hdr_size contains the bytes consumed by the DiskChunkHeader.		
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION1: null pointer\n";

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: not out of range for depth
	if(hdrp->depth < Chunk::MIN_DEPTH || hdrp->depth > maxDepth)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.1: depth out of range for a dir chunk\n";	
		
	//ASSERTION 1.2: not the root chunk
	if(hdrp->depth == Chunk::MIN_DEPTH)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.2: depth denotes the root chunk!\n";	
		
	//ASSERTION 1.3: not a data chunk
	if(hdrp->depth == maxDepth)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.3 depth denotes a data chunk!\n";	
				
	//begin by placing the static part of a DiskDirchunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDirChunk));
	currentp += sizeof(DiskDirChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDirChunk); // this is the size of the static part of a DiskDirChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION2: null pointer\n";	
	//first store the domains of the chunk id
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
	//loop invariant: a domain of the chunk id will be stored. A domain
	// is only a pointer to an array of order codes.
		DiskChunkHeader::Domain_t* dmnp = &(hdrp->chunk_id)[i];
		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
		hdr_size += sizeof(DiskChunkHeader::Domain_t);
		chnk_size += sizeof(DiskChunkHeader::Domain_t);
	}//end for		
	//Next store the order-codes of the domains
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
		//ASSERTION3: ordercodes pointer is not null
		if(!(hdrp->chunk_id)[i].ordercodes)
			throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION3: null pointer\n";			
		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
		//loop invariant: each ordercode of the current domain will stored
			DiskChunkHeader::ordercode_t* op = &((hdrp->chunk_id)[i].ordercodes[j]);
        		memcpy(currentp, reinterpret_cast<char*>(op), sizeof(DiskChunkHeader::ordercode_t));
        		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
        		chnk_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
		
	//next place the orcercode ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION4: null pointer\n";		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for

	//finally place the dir entries
       	for(int i =0; i<chnkp->hdr.no_entries; i++) { //for each entry
	       	DiskDirChunk::DirEntry_t*ep = &chnkp->entry[i];
        	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDirChunk::DirEntry_t));
        	currentp += sizeof(DiskDirChunk::DirEntry_t); // move on to the next empty position
        	chnk_size += sizeof(DiskDirChunk::DirEntry_t);
       	}//end for
}// end of AccessManager::placeDiskDirChunkInBcktBody

void traverseSingleCostTreeCreateChunkVectors(
				unsigned int maxDepth,
				unsigned int numFacts,
				const CostNode* const costRoot,
				const string& factFile,
				const BucketID& bcktID,
				vector<DirChunk>* &dirVectp,
				vector<DataChunk>* &dataVectp,
				const TreeTraversal_t howToTraverse)
//precondition:
//	costRoot points at a cost-tree and NOT a single data chunk,
//	dirVectp and dataVectp are NULL vectors
//	bcktID contains a valid bucket id where these tree will be eventually be stored in
//postcondition:
//	dirVectp and dataVectp point at two vectors containing the instantiated dir and Data chunks
//	respectively.
{
	// ASSERTION 1: costRoot does not point at a data chunk
       	if(costRoot->getchunkHdrp()->depth == maxDepth)
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 1: found single data chunk in input tree\n";

	// ASSERTION 2: null input pointers
	if(dirVectp || dataVectp)
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 2: not null vector pointers\n";

	// ASSERTION 3: not null bucket id
	if(bcktID.isnull())
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 3: found null input bucket id\n";

	// Reserve space for the two chunk vectors
	dirVectp = new vector<DirChunk>;
	unsigned int numDirChunks = 0;
	CostNode::countDirChunksOfTree(costRoot, maxDepth, numDirChunks);
	dirVectp->reserve(numDirChunks);

	dataVectp = new vector<DataChunk>;
	unsigned int numDataChunks = 0;
	CostNode::countDataChunksOfTree(costRoot, maxDepth, numDataChunks);
	dataVectp->reserve(numDataChunks);

	// Fill the chunk vectors by descending the cost tree with the appropriate
	// traversal method.
        switch(howToTraverse){
                case breadthFirst:
                        {
                        	queue<CostNode*> nextToVisit; // breadth-1st queue
                        	try {
                        		descendBreadth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
                        		        costRoot,
                        		        factFile,
                        		        bcktID,
                        		        nextToVisit,
                        		        dirVectp,
                        		        dataVectp);
                        	}
                              	catch(const char* message) {
                              		string msg("AccessManager::traverseSingleCostTreeCreateChunkVectors ==>");
                              		msg += message;
                              		throw msg;
                              	}
                      	}//end block
                      	break;
                case depthFirst:
                        {
                        	try {
                        		descendDepth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
        				        costRoot,
        				        factFile,
        				        bcktID,
        				        dirVectp,
        				        dataVectp);
                        	}
                              	catch(const char* message) {
                              		string msg("AccessManager::traverseSingleCostTreeCreateChunkVectors ==>");
                              		msg += message;
                              		throw msg;
                              	}
                      	}//end block
              		break;
                 default:
                        throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> Unkown traversal method\n";
                        break;
         }//end switch

      	//ASSERTION 4: valid returned vectors
       	if(dirVectp->empty() || dataVectp->empty())
       			throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 4: empty chunk vector!!\n";
}//end of AccessManager::traverseSingleCostTreeCreateChunkVectors

void placeChunksOfSingleTreeInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<DirChunk>* const dirVectp,
			const vector<DataChunk>* const dataVectp,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep,
			const TreeTraversal_t howToTraverse)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::traverseSingleCostTreeCreateChunkVectors. dbuckp (input parameter) is
//	a const pointer to an allocated DiskBucket, where its header members have been initialized.
//	Finally, nextFreeBytep is a byte pointer (input+output parameter) that points at the beginning
//	of free space in the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      traversal method of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        //According to the desired traversal method store the chunks in the body of the bucket
        switch(howToTraverse){
                case breadthFirst:
                        try{
                                _storeBreadth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(const char* message) {
                		string msg("AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> ");
                		msg += message;
                		throw msg.c_str();
                	}
                        break;

                case depthFirst:
                        try{
                                _storeDepth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(const char* message) {
                		string msg("AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> ");
                		msg += message;
                		throw msg.c_str();
                	}
                        break;
                default:
                        throw "AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> unknown traversal method\n";
                        break;
        }//end switch
}// end of AccessManager::placeChunksOfSingleTreeInDiskBucketBody