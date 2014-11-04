
#include "mesh.h"
#include "simplification.h"

#define BOUNDARY_COST 1.0


bool operator>(const EdgeCollapseTarget &a, const EdgeCollapseTarget &b){
    return a.cost > b.cost;
}



void Simplification::InitSimplification(Mesh *mesh_in)
{
    mesh = mesh_in;

    n_active_faces = mesh->n_faces;

    AssignInitialQ();

    for(EdgeIter ei = mesh->edges.begin(); ei != mesh->edges.end(); ei++)
        ComputeOptimalCoordAndCost(ei);
}


void Simplification::AssignInitialQ()
{
    for(VertexIter vi = mesh->vertices.begin(); vi != mesh->vertices.end(); vi++){

        for(int i = 0; i < 10; i++) vi->Q[i] = 0.0;

        HalfEdge *startHalfEdge, *endHalfEdge;

        if(vi->isBoundary == false) startHalfEdge = vi->neighborHe;
        else                        startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(vi->neighborHe);

        HalfEdge *hep = startHalfEdge;
        do{
            CumulateQ(vi, hep->face->normal, -DotProduct(hep->face->normal, hep->face->halfedge[0].vertex->coord));

            if(vi->isBoundary && hep->prev->mate == NULL){
                endHalfEdge = hep->prev;
                break;
            }

            hep = hep->prev->mate;
        }while(hep != startHalfEdge && hep != NULL);


        if(vi->isBoundary){
            // add pseudo face information to vi->Q

            double boundaryVector[3], pseudoNormal[3];

            for(int i = 0; i < 3; i++) boundaryVector[i] = startHalfEdge->next->vertex->coord[i] - startHalfEdge->vertex->coord[i]; 

            CrossProduct(boundaryVector, startHalfEdge->face->normal, pseudoNormal);
            Normalize(pseudoNormal);

            CumulateQ(vi, pseudoNormal, -DotProduct(pseudoNormal, startHalfEdge->vertex->coord));

            for(int i = 0; i < 3; i++) boundaryVector[i] = endHalfEdge->next->vertex->coord[i] - endHalfEdge->vertex->coord[i]; 

            CrossProduct(boundaryVector, endHalfEdge->face->normal, pseudoNormal);
            Normalize(pseudoNormal);

            CumulateQ(vi, pseudoNormal, -DotProduct(pseudoNormal, endHalfEdge->vertex->coord));
        }

    } // for(VertexIter vi = vertices.begin(); vi != vertices.end(); vi++){

}

void Simplification::CumulateQ(VertexIter &vi, double *normal, double d)
{
    double a = normal[0];
    double b = normal[1];
    double c = normal[2];

    vi->Q[0] += a*a; 
    vi->Q[1] += a*b; 
    vi->Q[2] += a*c;   
    vi->Q[3] += a*d;
    vi->Q[4] += b*b; 
    vi->Q[5] += b*c;  
    vi->Q[6] += b*d;
    vi->Q[7] += c*c; 
    vi->Q[8] += c*d;
    vi->Q[9] += d*d;
}

void Simplification::ComputeOptimalCoordAndCost(EdgeIter &ei)
{
    VertexIter v0 = ei->halfedge[0]->vertex;
    VertexIter v1 = ei->halfedge[0]->next->vertex;

    double newQ[4][4];

    newQ[0][0]              = v0->Q[0] + v1->Q[0];
    newQ[0][1] = newQ[1][0] = v0->Q[1] + v1->Q[1];
    newQ[0][2] = newQ[2][0] = v0->Q[2] + v1->Q[2];
    newQ[0][3] = newQ[3][0] = v0->Q[3] + v1->Q[3];
    newQ[1][1]              = v0->Q[4] + v1->Q[4];
    newQ[1][2] = newQ[2][1] = v0->Q[5] + v1->Q[5];
    newQ[1][3] = newQ[3][1] = v0->Q[6] + v1->Q[6];
    newQ[2][2]              = v0->Q[7] + v1->Q[7];
    newQ[2][3] = newQ[3][2] = v0->Q[8] + v1->Q[8];
    newQ[3][3]              = v0->Q[9] + v1->Q[9];

    double matrix[4][4], rhs[4] = { 0.0, 0.0, 0.0, 1.0 }, solution[4];

    for(int i = 0; i < 3; i++) for(int j = 0; j < 4; j++) matrix[i][j] = newQ[i][j];
    matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
    matrix[3][3] = 1.0;

    double cost;
    double optimalCoord[3];

    if( SolveLinearSystem(matrix, rhs, solution) ){
        double temp[4];
        for(int i = 0; i < 4; i++) temp[i] = DotProduct4D(newQ[i], solution);

        cost = DotProduct4D(solution, temp);
        for(int i = 0; i < 3; i++) optimalCoord[i] = solution[i];
    }else{ // matrix is singular. solution is not unique.
        cost = 0.0;
        if(v0->isBoundary) for(int i = 0; i < 3; i++) optimalCoord[i] = v0->coord[i];
        else               for(int i = 0; i < 3; i++) optimalCoord[i] = v1->coord[i];
    }

    // if "ei" is boundary, increase cost
    if(v0->isBoundary || v1->isBoundary) cost += BOUNDARY_COST;


    heap.push( EdgeCollapseTarget(ei, cost, optimalCoord, ect_id_base) );
    ei->ect_id = ect_id_base;

    ect_id_base++;
}


bool Simplification::EdgeCollapse()
{
    if(n_active_faces < 3) return false;

    // if "readdedEdgeCollapseTarget" is not empty, this must have the highest priority to collapse
    if( readdedEdgeCollapseTarget.empty() == false ){
        RemoveEdge(readdedEdgeCollapseTarget.top().ei, readdedEdgeCollapseTarget.top().optimalCoord, false);

        readdedEdgeCollapseTarget.pop();
        return true;
    }


    list<EdgeCollapseTarget>::iterator ecti = suspendedEdgeCollapseTarget.begin();
    while( ecti != suspendedEdgeCollapseTarget.end() ){

        if(ecti->ei->isActive == false || ecti->id != ecti->ei->ect_id){
            // obsolete. delete this
            ecti = suspendedEdgeCollapseTarget.erase(ecti);
        }else{
            if( IsFinWillNotBeCreated(ecti->ei) ){
                RemoveEdge(ecti->ei, ecti->optimalCoord, true);
                ecti = suspendedEdgeCollapseTarget.erase(ecti);
                return true;
            }else{
                ecti++;
            }
        }

    }

    EdgeCollapseTarget ect;

    while(heap.empty() == false){
        ect = heap.top();
        heap.pop(); 

        // ect.ei is an edge that is up-to-date
        if(ect.ei->isActive == true && ect.id == ect.ei->ect_id){
               
            if( IsFinWillNotBeCreated(ect.ei) ){    
                RemoveEdge(ect.ei, ect.optimalCoord, true); 
                return true;
            }
            else{            
                suspendedEdgeCollapseTarget.push_back(ect); 
            }
                  
        }

    }


    return false;
}


void Simplification::RemoveEdge(EdgeIter &ei, double *optimalCoord, bool isFirstCollapse)
{
    HalfEdge *hepCollapse = ei->halfedge[0];

    VertexIter v0 = hepCollapse->vertex;
    VertexIter v1 = hepCollapse->next->vertex;
   
    // inactivate removed faces
    hepCollapse->face->isActive = false;
    n_active_faces--;


    if(hepCollapse->mate != NULL){
        hepCollapse->mate->face->isActive = false;
        n_active_faces--;
    }
    
    vertexSplitTarget.push( VertexSplitTarget() ); 

    vertexSplitTarget.top().ei = ei;
    for(int i = 0; i < 3; i++) vertexSplitTarget.top().v1OrginalCoord[i] = v1->coord[i];
    vertexSplitTarget.top().v1OriginalIsBoundary = v1->isBoundary;


    HalfEdge *startHalfEdge;

    vector<FaceIter> facesOriginallyIncidentToV0OrV1; 

    if(v0->isBoundary == false) startHalfEdge = hepCollapse;
    else                        startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(hepCollapse);

    HalfEdge *hep = startHalfEdge;
    do{
        facesOriginallyIncidentToV0OrV1.push_back(hep->face);

        hep = hep->prev->mate;
    }while(hep != startHalfEdge && hep != NULL);

    if(v1->isBoundary == false) startHalfEdge = hepCollapse->next;
    else                        startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(hepCollapse->next);

    hep = startHalfEdge;
    do{
        facesOriginallyIncidentToV0OrV1.push_back(hep->face);

        hep = hep->prev->mate;
    }while(hep != startHalfEdge && hep != NULL);


    if(v0->isBoundary == false) startHalfEdge = hepCollapse;
    else                        startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(hepCollapse);

    // replace v0 of halfedges with v1
    hep = startHalfEdge;
    do{
        if(hep->face->isActive){
             hep->vertex = v1;

             vertexSplitTarget.top().halfedgesAroundV0.push_back(hep);
        }

        hep = hep->prev->mate;
    }while(hep != startHalfEdge && hep != NULL);

#ifdef DEBUG
    cerr << "e ";
#endif

    // move v1 to optimalCoord
    for(int i = 0; i < 3; i++)  v1->coord[i] = optimalCoord[i];


    if(isFirstCollapse){
        // add v0's "Q" to v1's "Q"
        for(int i = 0; i < 10; i++) v1->Q[i] += v0->Q[i];
    }

    /////////////////////////////////////////////////////////////////////////////
    // reassign mates of halfedges and corresponding edge information as well
    /////////////////////////////////////////////////////////////////////////////
    hepCollapse->edge->isActive = false;

    if(hepCollapse->next->mate != NULL) hepCollapse->next->mate->mate = hepCollapse->prev->mate;
    if(hepCollapse->prev->mate != NULL) hepCollapse->prev->mate->mate = hepCollapse->next->mate;

    hepCollapse->prev->edge->isActive = false;

    if(hepCollapse->next->edge->halfedge[0] == hepCollapse->next) 
        hepCollapse->next->edge->halfedge[0] = hepCollapse->prev->mate;
    else                                         
        hepCollapse->next->edge->halfedge[1] = hepCollapse->prev->mate;

    // edge->halfedge[0] should not be NULL
    if(hepCollapse->next->edge->halfedge[0] == NULL){
        if(hepCollapse->next->edge->halfedge[1] != NULL){
            // swap
            hepCollapse->next->edge->halfedge[0] = hepCollapse->next->edge->halfedge[1];
            hepCollapse->next->edge->halfedge[1] = NULL;
        }else{ // hepCollapse->next->edge->halfedge[0] == NULL and hepCollapse->next->edge->halfedge[1] == NULL
            // edge becomes degenerate
            hepCollapse->next->edge->isActive = false;
        }
    }

    if(hepCollapse->prev->mate != NULL) hepCollapse->prev->mate->edge = hepCollapse->next->edge;


    if(hepCollapse->mate != NULL){ // when "ect->ei" is not a boundary edge
        if(hepCollapse->mate->next->mate != NULL) hepCollapse->mate->next->mate->mate = hepCollapse->mate->prev->mate;
        if(hepCollapse->mate->prev->mate != NULL) hepCollapse->mate->prev->mate->mate = hepCollapse->mate->next->mate;

        hepCollapse->mate->next->edge->isActive = false;

        if(hepCollapse->mate->prev->edge->halfedge[0] == hepCollapse->mate->prev) 
            hepCollapse->mate->prev->edge->halfedge[0] = hepCollapse->mate->next->mate;
        else                                                      
            hepCollapse->mate->prev->edge->halfedge[1] = hepCollapse->mate->next->mate;

        // edge->halfedge[0] should not be NULL
        if(hepCollapse->mate->prev->edge->halfedge[0] == NULL){
            if(hepCollapse->mate->prev->edge->halfedge[1] != NULL){
                // swap
                hepCollapse->mate->prev->edge->halfedge[0] = hepCollapse->mate->prev->edge->halfedge[1];
                hepCollapse->mate->prev->edge->halfedge[1] = NULL;
            }else{ // hepCollapse->mate->prev->edge->halfedge[0] == NULL and hepCollapse->mate->prev->edge->halfedge[1] == NULL
                // edge becomes degenerate
                hepCollapse->mate->prev->edge->isActive = false;
            }
        }

        if(hepCollapse->mate->next->mate != NULL) hepCollapse->mate->next->mate->edge = hepCollapse->mate->prev->edge;
    
    }

   if( hepCollapse->next->edge->isActive == false && 
       (hepCollapse->mate == NULL || hepCollapse->mate->prev->edge->isActive == false) ){
       // face disappeared after edge collapsing
       return;
   }



    if(v0->isBoundary) v1->isBoundary = true;

    FindNeighborHalfEdge(v1, facesOriginallyIncidentToV0OrV1);

    if(v1->isBoundary == false) startHalfEdge = v1->neighborHe;
    else                        startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(v1->neighborHe);

    // update cost and optimal vertex coordinate of incident edges, and normals of incident faces, and incident vertices' neighborHe;
    hep = startHalfEdge;
    do{

       if(isFirstCollapse) ComputeOptimalCoordAndCost(hep->edge);

        mesh->AssignFaceNormal(hep->face);

        hep->next->vertex->neighborHe = hep->next;

        if(hep->prev->mate == NULL){
            if(isFirstCollapse) ComputeOptimalCoordAndCost(hep->prev->edge);    
            hep->prev->vertex->neighborHe = hep->prev;
            break;   
        }

        hep = hep->prev->mate;
    }while(hep != startHalfEdge && hep != NULL);


    // Finally, update vertex normals as well
    mesh->AssignVertexNormal(v1);

    hep = startHalfEdge;
    do{
        mesh->AssignVertexNormal(hep->next->vertex);

        if(hep->prev->mate == NULL){ 
            mesh->AssignVertexNormal(hep->prev->vertex);
            break;   
        }

        hep = hep->prev->mate;
    }while(hep != startHalfEdge);

}

HalfEdge* Simplification::FindBoundaryEdgeIncidentToVertexInCW(HalfEdge *baseHalfEdge)
{
    ///////////////////////////////////////////////////////////////
    // Find boundary edges incident to "baseHalfEdge->vertex" 
    ///////////////////////////////////////////////////////////////

    // find a boundary edge by checking incident edges in CW if it exists
    HalfEdge *hep = baseHalfEdge;
    do{
        if(hep->mate == NULL) return hep;

        hep = hep->mate->next;
    }while(hep != baseHalfEdge);

    // this should not happen
    return hep;
}


void Simplification::FindNeighborHalfEdge(VertexIter &v1, vector<FaceIter> &facesOriginallyIncidentToV0OrV1)
{
    // choose neighborHe of v1 from a face that is still active

    for(unsigned int i = 0; i < facesOriginallyIncidentToV0OrV1.size(); i++){
        if(facesOriginallyIncidentToV0OrV1[i]->isActive == true){
            for(int j = 0; j < 3; j++){
                if(facesOriginallyIncidentToV0OrV1[i]->halfedge[j].vertex == v1) {
                    v1->neighborHe = &(facesOriginallyIncidentToV0OrV1[i]->halfedge[j]);
                    break;
                }
            }
            break;
        }
    }
}



bool Simplification::IsFinWillNotBeCreated(EdgeIter &ei)
{
    HalfEdge *hepCollapse = ei->halfedge[0];

    VertexIter v0 = hepCollapse->vertex;
    VertexIter v1 = hepCollapse->next->vertex;

    HalfEdge *startHalfEdgeV0, *startHalfEdgeV1;

    // change neighborHE to make sure that it is outside of collapsed face
    if(v0->isBoundary == false) startHalfEdgeV0 = hepCollapse;
    else                        startHalfEdgeV0 = FindBoundaryEdgeIncidentToVertexInCW(hepCollapse);


    if(v1->isBoundary == false) startHalfEdgeV1 = hepCollapse->next;
    else                        startHalfEdgeV1 = FindBoundaryEdgeIncidentToVertexInCW(hepCollapse->next);

    HalfEdge *hepV0 = startHalfEdgeV0;
    do{
        HalfEdge *hepV1 = startHalfEdgeV1;
        do{
            if(hepV0->next->vertex == hepV1->next->vertex ||
               (hepV1->prev->mate == NULL && hepV0->next->vertex == hepV1->prev->vertex) ){
                VertexIter commonVertex = hepV0->next->vertex;
                
                if( commonVertex != hepCollapse->prev->vertex && 
                    (hepCollapse->mate != NULL && commonVertex != hepCollapse->mate->prev->vertex) ){

                    return false;
                }
            }

            if(hepV0->prev->mate == NULL){

                if(hepV0->prev->vertex == hepV1->next->vertex ||
                    (hepV1->prev->mate == NULL && hepV0->prev->vertex == hepV1->prev->vertex) ){
                        VertexIter commonVertex = hepV0->prev->vertex;

                        if( commonVertex != hepCollapse->prev->vertex && 
                            (hepCollapse->mate != NULL && commonVertex != hepCollapse->mate->prev->vertex) ){
                                return false;
                        }
                }

            } // if(hepV0->prev->mate == NULL){

            hepV1 = hepV1->prev->mate;
        }while(hepV1 != startHalfEdgeV1 && hepV1 != NULL);

        hepV0 = hepV0->prev->mate;
    }while(hepV0 != startHalfEdgeV0 && hepV0 != NULL);

        
    return true;
}




void Simplification::VertexSplit()
{
    if(vertexSplitTarget.empty() == false){

        HalfEdge *hepCollapsed = vertexSplitTarget.top().ei->halfedge[0];


        VertexIter v0 = hepCollapsed->vertex; 
        VertexIter v1 = hepCollapsed->next->vertex; 


        // re-add EdgeCollapseTarget in the stack before rechange "v1"
        readdedEdgeCollapseTarget.push( EdgeCollapseTarget(vertexSplitTarget.top().ei, -1, v1->coord, -1) );


        for(int i = 0; i < 3; i++) v1->coord[i] = vertexSplitTarget.top().v1OrginalCoord[i];
        v1->isBoundary = vertexSplitTarget.top().v1OriginalIsBoundary;

        hepCollapsed->edge->isActive = true;

        FaceIter f0 = hepCollapsed->face;

        f0->isActive = true;
        n_active_faces++;

        if(hepCollapsed->next->mate != NULL) hepCollapsed->next->mate->mate = hepCollapsed->next;
        if(hepCollapsed->prev->mate != NULL) hepCollapsed->prev->mate->mate = hepCollapsed->prev;

        hepCollapsed->prev->edge->isActive = true;

        if(hepCollapsed->next->edge->halfedge[0] == hepCollapsed->next->mate) 
            hepCollapsed->next->edge->halfedge[1] = hepCollapsed->next;
        else                 
            hepCollapsed->next->edge->halfedge[0] = hepCollapsed->next;
  

        for(int i = 0; i < 3; i++){
            f0->halfedge[i].vertex->neighborHe = &(f0->halfedge[i]);
        }

        if(hepCollapsed->mate != NULL){
            FaceIter f1 = hepCollapsed->mate->face;

            f1->isActive = true;
            n_active_faces++;

            if(hepCollapsed->mate->next->mate != NULL) hepCollapsed->mate->next->mate->mate = hepCollapsed->mate->next;
            if(hepCollapsed->mate->prev->mate != NULL) hepCollapsed->mate->prev->mate->mate = hepCollapsed->mate->prev;

            hepCollapsed->mate->prev->edge->isActive = true;

            if(hepCollapsed->mate->prev->edge->halfedge[0] == hepCollapsed->mate->prev->mate) 
                hepCollapsed->mate->prev->edge->halfedge[1] = hepCollapsed->mate->prev;
            else                 
                hepCollapsed->mate->prev->edge->halfedge[0] = hepCollapsed->mate->prev;

            for(int i = 0; i < 3; i++){
                f1->halfedge[i].vertex->neighborHe = &(f1->halfedge[i]);
            }

        }

        for(unsigned int i = 0; i < vertexSplitTarget.top().halfedgesAroundV0.size(); i++){
            HalfEdge *hep = vertexSplitTarget.top().halfedgesAroundV0[i];

            hep->vertex = v0;
        }


        // update normal vectors
        for(int i = 0; i < 2; i++){
            VertexIter v_target;

            if(i == 0) v_target = v0;
            else       v_target = v1;

            HalfEdge *startHalfEdge;

            if(v_target->isBoundary == false) startHalfEdge = v_target->neighborHe;
            else                              startHalfEdge = FindBoundaryEdgeIncidentToVertexInCW(v_target->neighborHe);

            // update cost and optimal vertex coordinate of incident edges, and normals of incident faces, and incident vertices' neighborHe;
            HalfEdge *hep = startHalfEdge;
            do{
                mesh->AssignFaceNormal(hep->face);

                hep = hep->prev->mate;
            }while(hep != startHalfEdge && hep != NULL);

            mesh->AssignVertexNormal(v_target);

            hep = startHalfEdge;
            do{
                mesh->AssignVertexNormal(hep->next->vertex);

                if(hep->prev->mate == NULL){ 
                    mesh->AssignVertexNormal(hep->prev->vertex);
                    break;   
                }

                hep = hep->prev->mate;
            }while(hep != startHalfEdge);

        } // for(int i = 0; i < 2; i++){



        vertexSplitTarget.pop();
    
    } // if(vertexSplitTarget.empty() == false){

}



void Simplification::ControlLevelOfDetail(int step)
{
    int n_target_faces = mesh->n_faces*pow(0.95, step);

    cerr << "step " << step << " " << n_target_faces << " " << mesh->n_faces << endl;

    if(n_target_faces < n_active_faces){
        while(n_target_faces < n_active_faces) if(EdgeCollapse() == false) break;
    }else if(n_target_faces > n_active_faces){
        while(n_target_faces > n_active_faces) VertexSplit();
    }

}

