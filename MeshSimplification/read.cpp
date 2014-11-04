

#include "mesh.h"
#include <cstdio>

bool Mesh::ConstructMeshDataStructure(char *filename)
{
    if( ReadOFFFile(filename) == false ) return false;
    AddEdgeInfo();

    return true;
}

bool Mesh::ReadOFFFile(char *filename)
{
    FILE *fp;

    if((fp = fopen(filename, "r")) == NULL ){
        cerr << "file cannot be read.\n";
        return false;
    }

    char buf[512];

    // discard the first line
    fgets(buf, 512, fp);

    // obtain the number of vertices and faces
    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", &n_vertices, &n_faces);

    vector<VertexIter> vertex_iterator;
    int v_id = 0, f_id = 0;

    for(int i = 0; i < n_vertices; i++){
        double coord_in[3];

        fgets(buf, 512, fp);
        sscanf(buf, "%lf%lf%lf", &coord_in[0], &coord_in[1], &coord_in[2]);

        vertices.push_back( Vertex(coord_in, v_id++) );
        vertex_iterator.push_back( --(vertices.end()) );
    }

    for(int i = 0; i < n_faces; i++){
        int v_id[3], dummy;

        fgets(buf, 512, fp);
        sscanf(buf, "%d%d%d%d", &dummy, &v_id[0], &v_id[1], &v_id[2]);

        faces.push_back( Face(vertex_iterator[ v_id[0] ], vertex_iterator[ v_id[1] ], vertex_iterator[ v_id[2] ], f_id++) );
    }

    cerr << "Reading Off file done...\n";

    fclose(fp);


    cerr << "# of vertices  " << n_vertices << endl;
    cerr << "# of faces     " << n_faces    << endl;


    double range_min[3] = {  1.0e6,  1.0e6,  1.0e6, };
    double range_max[3] = { -1.0e6, -1.0e6, -1.0e6, };
    double center[3];

    for(VertexIter vi = vertices.begin(); vi != vertices.end(); vi++){
        for(int i = 0; i < 3; i++){
            if(vi->coord[i] < range_min[i])	range_min[i] = vi->coord[i];
            if(vi->coord[i] > range_max[i])	range_max[i] = vi->coord[i];
        }
    }

    // set input model within (-1,-1,-1) and (1,1,1) and the cetroid is at the origin

    for(int i = 0; i < 3; i++) center[i] = (range_min[i] + range_max[i])*0.5;

    double largest_range = -1.0;

    for(int i = 0; i < 3; i++){
        if(largest_range < range_max[i]-range_min[i]) largest_range = range_max[i]-range_min[i];
    }

    double scale_factor = 2.0/largest_range;

    for(VertexIter vi = vertices.begin(); vi != vertices.end(); vi++){
        for(int i = 0; i < 3; i++){
            vi->coord[i] = (vi->coord[i] - center[i]) * scale_factor;
        }
    }


    return true;
}


void Mesh::MakeCircularList(FaceIter &fi)
{
    fi->halfedge[0].next = &(fi->halfedge[1]);
    fi->halfedge[1].next = &(fi->halfedge[2]);
    fi->halfedge[2].next = &(fi->halfedge[0]);

    fi->halfedge[0].prev = &(fi->halfedge[2]);
    fi->halfedge[1].prev = &(fi->halfedge[0]);
    fi->halfedge[2].prev = &(fi->halfedge[1]);
}

void Mesh::AddEdgeInfo()
{
    // store face iteretors incident to each vertex
    vector< vector<FaceIter> > Ring(n_vertices);

    for(FaceIter fi = faces.begin(); fi != faces.end(); fi++){
        // construct circular list
        MakeCircularList(fi);

        for(int i = 0; i < 3; i++){
            //fi->halfedge[i].vertex = fi->vertex[i];
            fi->halfedge[i].face = fi;
            fi->halfedge[i].vertex->neighborHe = &(fi->halfedge[i]);

            Ring[fi->halfedge[i].vertex->id].push_back(fi);
        }
    }

    cerr << "halfedges are set\n";

    // construct mates of halfedge
    for(unsigned int i = 0; i < Ring.size(); i++){ // for each vertex

        for(unsigned int j = 0; j < Ring[i].size(); j++){

            HalfEdge   *candidate_he;
            VertexIter candidate_vertex;

            for(int m = 0; m < 3; m++){
                if(Ring[i][j]->halfedge[m].vertex->id == i){
                    candidate_he     = &(Ring[i][j]->halfedge[m]);
                    candidate_vertex = Ring[i][j]->halfedge[m].next->vertex;
                    break;
                }
            }

            for(unsigned int k = 0; k < Ring[i].size(); k++){

                if(j==k) continue;

                for(int m = 0; m < 3; m++){
                    if( Ring[i][k]->halfedge[m].vertex == candidate_vertex &&
                        Ring[i][k]->halfedge[m].next->vertex->id == i ){
                            candidate_he->mate = &(Ring[i][k]->halfedge[m]);
                            Ring[i][k]->halfedge[m].mate = candidate_he;
                            break;
                    }
                }

            } //  for(int k = j+1; k < Ring[i].size(); k++){
        } // for(int j = 0; j < Ring[i].size(); j++){
    }

    cerr << "halfedge mates are set\n";


    // add edge information
    for(FaceIter fi = faces.begin(); fi != faces.end(); fi++){
      
        for(int i = 0; i < 3; i++){
            if( fi->halfedge[i].mate == NULL ||
                fi->halfedge[i].vertex->id < fi->halfedge[i].mate->vertex->id ){
                edges.push_back( Edge( &(fi->halfedge[i]), fi->halfedge[i].mate, n_edges++) );
            }
            if( fi->halfedge[i].mate == NULL ) fi->halfedge[i].vertex->isBoundary = true;
        } // for(int i = 0; i < 3; i++){

    }

    cerr << "edges are set\n";

    // construct link from halfedge to the corresponding edge
    for(EdgeIter ei = edges.begin(); ei != edges.end(); ei++){
        ei->halfedge[0]->edge = ei;
        if(ei->halfedge[1] != NULL) ei->halfedge[1]->edge = ei;
    }

    cerr << "# of edges "  << n_edges << endl;

    for(FaceIter   fi = faces.begin();    fi != faces.end();    fi++) AssignFaceNormal(fi);
    for(VertexIter vi = vertices.begin(); vi != vertices.end(); vi++) AssignVertexNormal(vi);
}

void Mesh::AssignFaceNormal(FaceIter &fi)
{
    double vec1[3], vec2[3];

    for(int i = 0; i < 3; i++){
        vec1[i] = fi->halfedge[1].vertex->coord[i] - fi->halfedge[0].vertex->coord[i];
        vec2[i] = fi->halfedge[2].vertex->coord[i] - fi->halfedge[0].vertex->coord[i];
    }

    CrossProduct(vec1, vec2, fi->normal);
    GetArea(fi->normal, fi->area);
    Normalize(fi->normal);
}

void Mesh::AssignVertexNormal(VertexIter &vi)
{
    bool isBoundaryVertex = false;

    vi->normal[0] = vi->normal[1] = vi->normal[2] = 0.0;
    double cumulativeArea = 0.0;

    // traverse faces incident to "vi" in CCW
    HalfEdge *hep = vi->neighborHe;
    do{
        FaceIter fi = hep->face;

        vi->normal[0] += fi->normal[0]*fi->area;
        vi->normal[1] += fi->normal[1]*fi->area;
        vi->normal[2] += fi->normal[2]*fi->area;
        cumulativeArea += fi->area;

        hep = hep->prev->mate;

        if(hep == NULL){
            isBoundaryVertex = true;
            break;
        }
    }while(hep != vi->neighborHe);

    // when we cannot traverse all incident faces since "vi" is on boundary
    // we traverse faces incident to "vi" in CW to check all incident faces
    if(isBoundaryVertex){
        HalfEdge *hep = vi->neighborHe->mate;
        while(hep != NULL){
            FaceIter fi = hep->face;

            vi->normal[0] += fi->normal[0]*fi->area;
            vi->normal[1] += fi->normal[1]*fi->area;
            vi->normal[2] += fi->normal[2]*fi->area;
            cumulativeArea += fi->area;

            hep = hep->next->mate;
        }
    }

    double invCumulativeArea = 1.0 / cumulativeArea;

    vi->normal[0] *= invCumulativeArea;
    vi->normal[1] *= invCumulativeArea;
    vi->normal[2] *= invCumulativeArea;
}

