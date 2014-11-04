
#include "mesh.h"

void Mesh::Display(int mode)
{
    glEnable(GL_LIGHTING);
    glEnable( GL_POLYGON_OFFSET_FILL ); 
    glPolygonOffset(1.0, 1.0);

    if(mode == 0){

        glEnable(GL_LIGHTING);

        for(FaceIter fi = faces.begin(); fi != faces.end(); fi++){ 

            if(fi->isActive){
                glBegin(GL_TRIANGLES);
                glNormal3dv(fi->halfedge[0].vertex->normal);
                glVertex3dv(fi->halfedge[0].vertex->coord);
                glNormal3dv(fi->halfedge[1].vertex->normal);
                glVertex3dv(fi->halfedge[1].vertex->coord);
                glNormal3dv(fi->halfedge[2].vertex->normal);
                glVertex3dv(fi->halfedge[2].vertex->coord);
                glEnd();
            }
        }

    }else if(mode == 1){

        for(FaceIter fi = faces.begin(); fi != faces.end(); fi++){ 

            if(fi->isActive){
                glBegin(GL_TRIANGLES);
                glNormal3dv(fi->halfedge[0].vertex->normal);
                glVertex3dv(fi->halfedge[0].vertex->coord);
                glNormal3dv(fi->halfedge[1].vertex->normal);
                glVertex3dv(fi->halfedge[1].vertex->coord);
                glNormal3dv(fi->halfedge[2].vertex->normal);
                glVertex3dv(fi->halfedge[2].vertex->coord);
                glEnd();
            }
        }

        glDisable(GL_LIGHTING);
        glLineWidth(1.0);
        glColor3d(0, 0, 0);

        for(FaceIter fi = faces.begin(); fi != faces.end(); fi++){ 
            if(fi->isActive){
                glBegin(GL_LINE_LOOP);
                glVertex3dv(fi->halfedge[0].vertex->coord);
                glVertex3dv(fi->halfedge[1].vertex->coord);
                glVertex3dv(fi->halfedge[2].vertex->coord);
                glEnd();
            }
        }

    }


}
