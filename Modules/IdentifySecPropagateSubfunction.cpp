/* propagate.c - */

/* The contents of this file are subject to the Mozilla Public License Version 
 * 1.1 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at 
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 * 
 * 
 * The Original Code is the Identify Secondary Objects Propagate Subfunction.
 * 
 * The Initial Developer of the Original Code is
 * Whitehead Institute for Biological Research
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 * 
 * Contributor(s):
 *   Anne Carpenter <carpenter@wi.mit.edu>
 *   Thouis Jones   <thouis@csail.mit.edu>
 *   In Han Kang    <inthek@mit.edu>
 *
 * $Revision$
 */

#include <math.h>
#include "mex.h"
#include <queue>
#include <vector>
#include <iostream>
using namespace std;

/* Input Arguments */
#define LABELS_IN       prhs[0]
#define IM_IN           prhs[1]
#define MASK_IN         prhs[2]
#define LAMBDA_IN       prhs[3]

/* Output Arguments */
#define LABELS_OUT        plhs[0]

#define IJ(i,j) ((j)*m+(i))

class Pixel { 
public:
  double distance;
  unsigned int i, j;
  double label;
  Pixel (double ds, unsigned int ini, unsigned int inj, double l) : 
    distance(ds), i(ini), j(inj), label(l) {}
};

struct Pixel_compare { 
 bool operator() (const Pixel& a, const Pixel& b) const 
 { return a.distance > b.distance; }
};

typedef priority_queue<Pixel, vector<Pixel>, Pixel_compare> PixelQueue;

static double
clamped_fetch(double *image, 
              int i, int j,
              int m, int n)
{
  if (i < 0) i = 0;
  if (i == m) i = m-1;
  if (j < 0) j = 0;
  if (j == n) j = n-1;

  return (image[IJ(i,j)]);
}

static double
Difference(double *image,
           int i1,  int j1,
           int i2,  int j2,
           unsigned int m, unsigned int n,
           double lambda)
{
  int delta_i, delta_j;
  double pixel_diff = 0.0;

  for (delta_j = -1; delta_j < 2; delta_j++) {
    for (delta_i = -1; delta_i < 2; delta_i++) {
      pixel_diff += fabs(clamped_fetch(image, i1 + delta_i, j1 + delta_j, m, n) - 
                         clamped_fetch(image, i2 + delta_i, j2 + delta_j, m, n));
    }
  }

  return (sqrt(pixel_diff*pixel_diff + (fabs((double) i1 - i2) + fabs((double) j1 - j2)) * lambda * lambda));
}

static void
push_neighbors_on_queue(PixelQueue &pq, double dist,
                        double *image,
                        unsigned int i, unsigned int j,
                        unsigned int m, unsigned int n,
                        double lambda, double label)
{
  /* 4-connected */
  if (i > 0) {
    pq.push(Pixel(dist + Difference(image, i, j, i-1, j, m, n, lambda), i-1, j, label));
  }                                                                   
  if (j > 0) {                                                        
    pq.push(Pixel(dist + Difference(image, i, j, i, j-1, m, n, lambda), i, j-1, label));
  }                                                                   
  if (i < (m-1)) {                                                    
    pq.push(Pixel(dist + Difference(image, i, j, i+1, j, m, n, lambda), i+1, j, label));
  }                                                                   
  if (j < (n-1)) {                                                    
    pq.push(Pixel(dist + Difference(image, i, j, i, j+1, m, n, lambda), i, j+1, label));
  } 

  /* 8-connected */
  if ((i > 0) && (j > 0)) {
    pq.push(Pixel(dist + Difference(image, i, j, i-1, j-1, m, n, lambda), i-1, j-1, label));
  }                                                                       
  if ((i < (m-1)) && (j > 0)) {                                           
    pq.push(Pixel(dist + Difference(image, i, j, i+1, j-1, m, n, lambda), i+1, j-1, label));
  }                                                                       
  if ((i > 0) && (j < (n-1))) {                                           
    pq.push(Pixel(dist + Difference(image, i, j, i-1, j+1, m, n, lambda), i-1, j+1, label));
  }                                                                       
  if ((i < (m-1)) && (j < (n-1))) {                                       
    pq.push(Pixel(dist + Difference(image, i, j, i+1, j+1, m, n, lambda), i+1, j+1, label));
  }
  
}

static void propagate(double *labels_in, double *im_in,
                      mxLogical *mask_in, double *labels_out,
                      unsigned int m, unsigned int n,
                      double lambda)
{
  unsigned int i, j;
  double *dists;
  mxArray *dist_array;

  PixelQueue pixel_queue;

  dist_array = mxCreateDoubleMatrix(m, n, mxREAL);
  dists = mxGetPr(dist_array);

  for (j = 0; j < n; j++) {
    for (i = 0; i < m; i++) {
      dists[IJ(i,j)] = mxGetInf();
      
      double label = labels_in[IJ(i,j)];
      labels_out[IJ(i,j)] = label;
      if ((label > 0) && (mask_in[IJ(i,j)])) {
        dists[IJ(i,j)] = 0.0;
        push_neighbors_on_queue(pixel_queue, 0.0, im_in, i, j, m, n, lambda, label);
      }
    }
  }

  while (! pixel_queue.empty()) {
    Pixel p = pixel_queue.top();
    pixel_queue.pop();
    
    //    cout << "popped " << p.i << " " << p.j << endl;

    
    if (! mask_in[IJ(p.i, p.j)]) continue;
    //    cout << "going on\n";

    if ((dists[IJ(p.i, p.j)] > p.distance) && (mask_in[IJ(p.i,p.j)])) {
      dists[IJ(p.i, p.j)] = p.distance;
      labels_out[IJ(p.i, p.j)] = p.label;
      push_neighbors_on_queue(pixel_queue, p.distance, im_in, p.i, p.j, m, n, lambda, p.label);
    }
  }

  mxDestroyArray(dist_array);
}

void mexFunction( int nlhs, mxArray *plhs[], 
                  int nrhs, const mxArray*prhs[] )
     
{ 
    double *labels_in, *im_in; 
    mxLogical *mask_in;
    double *labels_out;
    double *lambda; 
    unsigned int m, n; 
    
    /* Check for proper number of arguments */
    
    if (nrhs != 4) { 
        mexErrMsgTxt("Four input arguments required."); 
    } else if (nlhs > 1) {
        mexErrMsgTxt("Too many output arguments."); 
    } 

    m = mxGetM(IM_IN); 
    n = mxGetN(IM_IN);

    if ((m != mxGetM(LABELS_IN)) ||
        (n != mxGetN(LABELS_IN))) {
      mexErrMsgTxt("First and second arguments must have same size.");
    }

    if ((m != mxGetM(MASK_IN)) ||
        (n != mxGetN(MASK_IN))) {
      mexErrMsgTxt("First and third arguments must have same size.");
    }

    if (! mxIsDouble(LABELS_IN)) {
      mexErrMsgTxt("First argument must be a double array.");
    }
    if (! mxIsDouble(IM_IN)) {
      mexErrMsgTxt("Second argument must be a double array.");
    }
    if (! mxIsLogical(MASK_IN)) {
      mexErrMsgTxt("Third argument must be a logical array.");
    }

    /* Create a matrix for the return argument */ 
    LABELS_OUT = mxCreateDoubleMatrix(m, n, mxREAL); 
    
    /* Assign pointers to the various parameters */ 
    labels_in = mxGetPr(LABELS_IN);
    im_in = mxGetPr(IM_IN);
    mask_in = mxGetLogicals(MASK_IN);
    labels_out = mxGetPr(LABELS_OUT);
    lambda = mxGetPr(LAMBDA_IN);

    /* Do the actual computations in a subroutine */
    propagate(labels_in, im_in, mask_in, labels_out, m, n, *lambda); 
    return;
    
}
