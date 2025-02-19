#include "quat.h"

#define NUM_VERT 120 
#define NUM_EDGE 720
#define NUM_FACE 1200
#define NUM_CELL 600
// number of nearest neighbors = NUM_EDGE / NUM_VERT * 2
#define NNN 12

struct q_point{
	int vec[4][2] ;
	double weight ;
} ;

static double vertices[NUM_VERT][4] ;
static int edges[NUM_EDGE][2] ;
static int faces[NUM_FACE][3] ;
static int cells[NUM_CELL][4] ;
static int nn_list[NUM_VERT][NNN] ;
static int edge2cell[NUM_EDGE][4] ;
static int face2cell[NUM_FACE][4] ;
static int vec_vertices[NUM_VERT][4][2] ;

static struct q_point *vertice_points, *edge_points, *face_points, *cell_points ;
static int num_edge_point, num_face_point, num_cell_point ;

static void ver_even_permute(int num, int idx) {
	int i, j, k, m, n ;
	
	// even permutations
	int perm_idx[12][4] = {{0, 1, 2, 3}, {0, 2, 3, 1}, {0, 3, 1, 2},
	                       {1, 2, 0, 3}, {1, 0, 3, 2}, {1, 3, 2, 0},
	                       {2, 0, 1, 3}, {2, 3, 0, 1}, {2, 1, 3, 0},
	                       {3, 1, 0, 2}, {3, 0, 2, 1}, {3, 2, 1, 0}} ;
	double vert[4] ;
	int vec_vert[4][2] ;
	double tau = (sqrt(5.) + 1.) / 2. ;
	
	for (i = 0 ; i < 2 ; i++)
	for (j = 0 ; j < 2 ; j++)
	for (k = 0 ; k < 2 ; k++) {
		vert[0] = (2*i - 1)*tau/2. ;
		vert[1] = (2*j - 1)*0.5 ;
		vert[2] = (2*k - 1)/(2*tau) ;
		vert[3] = 0 ;
		
		vec_vert[0][0] = 0 ;
		vec_vert[0][1] = (2*i - 1)*num ;
		vec_vert[1][0] = (2*j - 1)*num ;
		vec_vert[1][1] = 0 ;
		vec_vert[2][0] = -(2*k - 1)*num ;
		vec_vert[2][1] = (2*k - 1)*num ;
		vec_vert[3][0] = 0 ;
		vec_vert[3][1] = 0 ;
		
		for (m = 0 ; m < 12 ; m++) {
			for (n = 0 ; n < 4 ; n++) {
				vertices[idx][n] = vert[perm_idx[m][n]] ;
				vec_vertices[idx][n][0] = vec_vert[perm_idx[m][n]][0] ;
				vec_vertices[idx][n][1] = vec_vert[perm_idx[m][n]][1] ;
			}
			idx += 1 ;
		}
	}
}	

static void make_vertex(int num) {
	int h, i, j, k, idx = 0 ;
	
	// 16 vertices
	for (h = 0 ; h < 2 ; h++) 
	for (i = 0 ; i < 2 ; i++) 
	for (j = 0 ; j < 2 ; j++) 
	for (k = 0 ; k < 2 ; k++) {
		vertices[idx][0] = h - 0.5 ;
		vertices[idx][1] = i - 0.5 ;
		vertices[idx][2] = j - 0.5 ;
		vertices[idx][3] = k - 0.5 ;
		
		vec_vertices[idx][0][0] = (2*h - 1)*num ;
		vec_vertices[idx][1][0] = (2*i - 1)*num ;
		vec_vertices[idx][2][0] = (2*j - 1)*num ;
		vec_vertices[idx][3][0] = (2*k - 1)*num ;
		
		vec_vertices[idx][0][1] = 0 ;
		vec_vertices[idx][1][1] = 0 ;
		vec_vertices[idx][2][1] = 0 ;
		vec_vertices[idx][3][1] = 0 ;
		idx += 1 ;
	}
	
	// 8 vertices
	for (h = 0 ; h < 2 ; h++)
	for (i = 0 ; i < 4 ; i++) {
		for (j = 0 ; j < 4 ; j++) {
			if (j == i) {
				vertices[idx][j] = 2*h - 1.0 ;
				vec_vertices[idx][j][0] = (2*h - 1)*2*num ;
				vec_vertices[idx][j][1] = 0 ;
			}
			else{
				vertices[idx][j] = 0 ;
				vec_vertices[idx][j][0] = 0 ;
				vec_vertices[idx][j][1] = 0 ;
			}
		}
		
		idx += 1 ;
	}
	
	// the rest 96 vertices
	ver_even_permute(num, idx) ;
}

static double calc_min_dist2(void) {
	double val = 0. ;
	int i, j ;
	
	for (i = 0 ; i < 4 ; i++)
		val += pow(vertices[0][i] - vertices[1][i], 2) ;
	
	for (i = 2 ; i < NUM_VERT ; i++) {
		double tmp = 0 ;
		for (j = 0 ; j < 4 ; j++)
			tmp += pow(vertices[0][j] - vertices[i][j], 2) ;
		if (tmp < val)
			val = tmp ;
	}
	
	// offset by a small number to avoid the round-off error
	return val + 1.e-6 ;
}
	
static void make_edge(int num, double min_dist2) {
	double tmp ;
	int i, j, k, edge_count = 0 ;
	int nn_count[NUM_VERT] ;
	
	for (i = 0 ; i < NUM_VERT ; i++)
		nn_count[i] = 0 ;
	
	for (i = 0 ; i < NUM_VERT ; i++)
	for (j = i + 1 ; j < NUM_VERT ; j++) {
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[i][k] - vertices[j][k], 2) ;
		
		if (tmp < min_dist2) {
			// edges[*][0] < edges[*][1]
			edges[edge_count][0] = i ;
			edges[edge_count][1] = j ;
			
			nn_list[i][nn_count[i]] = j ;
			nn_list[j][nn_count[j]] = i ;
			nn_count[i] += 1 ;
			nn_count[j] += 1 ;
			
			edge_count += 1 ;
		}
	}
}

static void make_face(int num, double min_dist2) {
	int i, j, k, idx ;
	int face_count = 0 ;
	double tmp ;
	
	for (i = 0 ; i < NUM_EDGE ; i++)
	for (j = 0 ; j < NNN ; j++) {
		if (nn_list[edges[i][0]][j] <= edges[i][1])
			continue ;
		
		idx = nn_list[edges[i][0]][j] ;
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[edges[i][1]][k], 2) ;
		
		if (tmp < min_dist2) {
			// faces[*][0] < faces[*][1] < faces[*][2]
			faces[face_count][0] = edges[i][0] ;
			faces[face_count][1] = edges[i][1];
			faces[face_count][2] = idx ;
			face_count += 1 ;
		}
	}
}

static void make_cell(int num, double min_dist2) {
	int i, j, k, idx ;
	int cell_count = 0 ;
	double tmp ;
	
	for (i = 0 ; i < NUM_FACE ; i++)
	for (j = 0 ; j < NNN ; j++) {
		if (nn_list[faces[i][0]][j] <= faces[i][2])
			continue ;
		
		idx = nn_list[faces[i][0]][j] ;
		
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[faces[i][1]][k], 2) ;
		
		if (tmp > min_dist2)
			continue ;
		
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[faces[i][2]][k], 2) ;
		
		if (tmp > min_dist2)
			continue ;
		
		// cells[*][0] < cells[*][1] < cells[*][2] < cells[*][3]
		cells[cell_count][0] = faces[i][0] ;
		cells[cell_count][1] = faces[i][1] ;
		cells[cell_count][2] = faces[i][2] ;
		cells[cell_count][3] = idx ;
		cell_count += 1 ;
	}
}

static void make_map(double min_dist2) {
	int i, j, k, m, idx ;
	double tmp ;
	
	// face2cell
	for (i = 0 ; i < NUM_FACE ; i++)
	for (j = 0 ; j < NNN ; j++) {
		idx = nn_list[faces[i][0]][j] ;
		if (idx == faces[i][1] || idx == faces[i][2])
			continue ;
		
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[faces[i][1]][k], 2) ;
		
		if (tmp > min_dist2)
			continue ;
		
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[faces[i][2]][k], 2) ;
		
		if (tmp > min_dist2)
			continue ;
		
		face2cell[i][0] = faces[i][0] ;
		face2cell[i][1] = faces[i][1] ;
		face2cell[i][2] = faces[i][2] ;
		face2cell[i][3] = idx ;
		
		break ;
	}
	
	// edge2cell
	for (i = 0 ; i < NUM_EDGE ; i++)
	for (j = 0 ; j < NNN ; j++) {
		idx = nn_list[edges[i][0]][j] ;
		if (idx == edges[i][1])
			continue ;
		
		tmp = 0 ;
		for (k = 0 ; k < 4 ; k++)
			tmp += pow(vertices[idx][k] - vertices[edges[i][1]][k], 2) ;
		
		if (tmp > min_dist2)
			continue ;
		
		edge2cell[i][0] = edges[i][0] ;
		edge2cell[i][1] = edges[i][1] ;
		edge2cell[i][2] = idx ;
	
		for (k = j + 1 ; k < NNN ; k++) {
			idx = nn_list[edges[i][0]][k] ;
			if (idx == edge2cell[i][1])
				continue ;
			
			tmp = 0 ;
			for (m = 0 ; m < 4 ; m++)
				tmp += pow(vertices[idx][m] - vertices[edge2cell[i][1]][m], 2) ;
			if (tmp > min_dist2)
				continue ;
			
			tmp = 0 ;
			for (m = 0 ; m < 4 ; m++)
				tmp += pow(vertices[idx][m] - vertices[edge2cell[i][2]][m], 2) ;
			if (tmp > min_dist2)
				continue ;
			
			edge2cell[i][3] = idx ;
			
			break ;
		}
		
		break ;
	}
}

static double weight( double *v_q, double *v_c ) {
	int i ;
	double w = 0, norm_q = 0, norm_c = 0 ;
	
	for (i = 0 ; i < 4 ; i++) {
		norm_q += pow(v_q[i], 2) ;
		norm_c += pow(v_c[i], 2) ;
	}
	
	norm_q = sqrt(norm_q) ;
	norm_c = sqrt(norm_c) ;
	
	for (i = 0 ; i < 4 ; i++)
		w += v_q[i]*v_c[i] ;
	
	w /= pow(norm_q, 4)*norm_c ;
	
	return w ;
}

static void quat_setup(int num, double **quat_ptr, int *num_rot_ptr) {
	int i, j, k, m, visited_vert[NUM_VERT] ;
	double v_q[4], v_c[4], w ;
	double f0 = 5./6 ;
	
	*num_rot_ptr = 10*(5*num*num*num + num) ;
	*quat_ptr = malloc((*num_rot_ptr) * 5 * sizeof(double)) ;
	vertice_points = malloc(NUM_VERT * sizeof(struct q_point)) ;
	num_edge_point = 0 ;
	num_face_point = 0 ;
	num_cell_point = 0 ;
	
	for (i = 0 ; i < NUM_VERT ; i++)
		visited_vert[i] = 0 ;
	
	for (i = 0 ; i < NUM_CELL ; i++)
	for (j = 0 ; j < 4 ; j++) {
		if (visited_vert[cells[i][j]] == 1)
			continue ;
		
		visited_vert[cells[i][j]] = 1 ;
		
		for (k = 0 ; k < 4 ; k++) {
			for (m = 0 ; m < 2 ; m++)
				vertice_points[cells[i][j]].vec[k][m] = vec_vertices[cells[i][j]][k][m] ;
		}
		
		for (k = 0 ; k < 4 ; k++) {
			v_c[k] = 0. ;
			for (m = 0 ; m < 4 ; m++)
				v_c[k] += vertices[cells[i][m]][k] ;
			v_q[k] = vertices[cells[i][j]][k] ;
		}
		
		w = f0*weight(v_q, v_c) ;
		vertice_points[cells[i][j]].weight = w ;
	}
}

static void refine_edge(int num) {
	int i, j, k ;
	double v_q[4], v_c[4], w ;
	double tau = (sqrt(5.) + 1.) / 2. ;
	int vec_d_v[4][2], edge_point_count = 0 ;
	double f1 = 35./36 ;
	
	num_edge_point = NUM_EDGE*(num - 1) ;
	edge_points = malloc(num_edge_point * sizeof(struct q_point)) ;
	
	for (i = 0 ; i < NUM_EDGE ; i++) {
		for (j = 0 ; j < 4 ; j++) {
			vec_d_v[j][0] = (vec_vertices[edges[i][1]][j][0] - vec_vertices[edges[i][0]][j][0]) / num ;
			vec_d_v[j][1] = (vec_vertices[edges[i][1]][j][1] - vec_vertices[edges[i][0]][j][1]) / num ;
		}
		
		for (j = 0 ; j < 4 ; j++) {
			v_c[j] = 0. ;
			for (k = 0 ; k < 4 ; k++)
				v_c[j] += vertices[edge2cell[i][k]][j] ;
		}
		
		for (j = 1 ; j < num ; j++) {
			for (k = 0 ; k < 4 ; k++) {
				edge_points[edge_point_count].vec[k][0] = vec_vertices[edges[i][0]][k][0] + j*vec_d_v[k][0] ;
				edge_points[edge_point_count].vec[k][1] = vec_vertices[edges[i][0]][k][1] + j*vec_d_v[k][1] ;
				v_q[k] = (edge_points[edge_point_count].vec[k][0] + tau*edge_points[edge_point_count].vec[k][1]) / (2.0*num) ;
			}
			
			w = f1*weight(v_q, v_c) ;
			edge_points[edge_point_count].weight = w ;
			edge_point_count += 1 ;
		}
	}
}

static void refine_face(int num) {
	int i, j, k, m ;
	double v_q[4], v_c[4], w ;
	double tau = (sqrt(5.) + 1.) / 2. ;
	int vec_d_v1[4][2], vec_d_v2[4][2], face_point_count = 0 ;
	
	num_face_point = NUM_FACE*(num - 2)*(num - 1)/2 ;
	face_points = malloc(num_face_point * sizeof(struct q_point)) ;
	
	for (i = 0 ; i < NUM_FACE ; i++) {
		for (j = 0 ; j < 4 ; j++) {
			vec_d_v1[j][0] = (vec_vertices[faces[i][1]][j][0] - vec_vertices[faces[i][0]][j][0]) / num ;
			vec_d_v1[j][1] = (vec_vertices[faces[i][1]][j][1] - vec_vertices[faces[i][0]][j][1]) / num ;
			vec_d_v2[j][0] = (vec_vertices[faces[i][2]][j][0] - vec_vertices[faces[i][0]][j][0]) / num ;
			vec_d_v2[j][1] = (vec_vertices[faces[i][2]][j][1] - vec_vertices[faces[i][0]][j][1]) / num ;
		}
		
		for (j = 0 ; j < 4 ; j++) {
			v_c[j] = 0. ;
			for (k = 0 ; k < 4 ; k++)
				v_c[j] += vertices[face2cell[i][k]][j] ;
		}
		
		for (j = 1 ; j < num - 1 ; j++)
		for (k = 1 ; k < num - j ; k++) {
			for (m = 0 ; m < 4 ; m++) {
				face_points[face_point_count].vec[m][0] = vec_vertices[faces[i][0]][m][0] + j*vec_d_v1[m][0] + k*vec_d_v2[m][0] ;
				face_points[face_point_count].vec[m][1] = vec_vertices[faces[i][0]][m][1] + j*vec_d_v1[m][1] + k*vec_d_v2[m][1] ;
				v_q[m] = (face_points[face_point_count].vec[m][0] + tau*face_points[face_point_count].vec[m][1]) / (2.0*num) ;
			}
			
			w = weight(v_q, v_c) ;
			face_points[face_point_count].weight = w ;
			face_point_count += 1 ;
		}
	}
}

static void refine_cell(int num) {
	int i, j, k, m, n ;
	double v_q[4], v_c[4], w ;
	double tau = (sqrt(5.) + 1.) / 2. ;
	int vec_d_v1[4][2], vec_d_v2[4][2], vec_d_v3[4][2], cell_point_count = 0 ;
	
	num_cell_point = NUM_CELL*(num - 3)*(num - 2)*(num - 1)/6 ;
	cell_points = malloc(num_cell_point * sizeof(struct q_point)) ;
	
	for (i = 0 ; i < NUM_CELL ; i++) {
		for (j = 0 ; j < 4 ; j++) {
			vec_d_v1[j][0] = (vec_vertices[cells[i][1]][j][0] - vec_vertices[cells[i][0]][j][0]) / num ;
			vec_d_v1[j][1] = (vec_vertices[cells[i][1]][j][1] - vec_vertices[cells[i][0]][j][1]) / num ;
			vec_d_v2[j][0] = (vec_vertices[cells[i][2]][j][0] - vec_vertices[cells[i][0]][j][0]) / num ;
			vec_d_v2[j][1] = (vec_vertices[cells[i][2]][j][1] - vec_vertices[cells[i][0]][j][1]) / num ;
			vec_d_v3[j][0] = (vec_vertices[cells[i][3]][j][0] - vec_vertices[cells[i][0]][j][0]) / num ;
			vec_d_v3[j][1] = (vec_vertices[cells[i][3]][j][1] - vec_vertices[cells[i][0]][j][1]) / num ;
		}
		
		for (j = 0 ; j < 4 ; j++) {
			v_c[j] = 0. ;
			for (k = 0 ; k < 4 ; k++)
				v_c[j] += vertices[cells[i][k]][j] ;
		}
		
		for (j = 1 ; j < num - 2 ; j++)
		for (k = 1 ; k < num - 1 - j ; k++)
		for (m = 1 ; m < num - j - k ; m++) {
			for (n = 0 ; n < 4 ; n++) {
				cell_points[cell_point_count].vec[n][0] = vec_vertices[cells[i][0]][n][0] + j*vec_d_v1[n][0] + k*vec_d_v2[n][0] + m*vec_d_v3[n][0] ;
				cell_points[cell_point_count].vec[n][1] = vec_vertices[cells[i][0]][n][1] + j*vec_d_v1[n][1] + k*vec_d_v2[n][1] + m*vec_d_v3[n][1] ;
				v_q[n] = (cell_points[cell_point_count].vec[n][0] + tau*cell_points[cell_point_count].vec[n][1]) / (2.0*num) ;
			}
			
			w = weight(v_q, v_c) ;
			cell_points[cell_point_count].weight = w ;
			cell_point_count += 1 ;
		}
	}
}

static void print_quat(int num, double *quat) {
	int r, rf, i, j, num_rot_test, flag, ct ;
	double q_v[4], q_norm ;
	double tau = (sqrt(5.) + 1.) / 2. ;
	
	num_rot_test = (NUM_VERT + num_edge_point + num_face_point + num_cell_point) / 2 ;
	if (num_rot_test != 10*(5*num*num*num + num)) {
		fprintf(stderr, "Inconsistency in calculation of num_rot.\n") ;
		return ;
	}
	
	// select half of the quaternions on vertices
	ct = 0 ;
	for (r = 0 ; r < NUM_VERT ; r++) {
		flag = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			for (j = 0 ; j < 2; j++) {
				if (vertice_points[r].vec[i][j] == 0)
					continue ;
				else if (vertice_points[r].vec[i][j] > 0) {
					flag = 1 ;
					break ;
				}
				else{
					flag = -1 ;
					break ;
				}
			}
			
			if (flag != 0)
				break ;
		}
		
		if (flag != 1)
			continue ;
		
		q_norm = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			q_v[i] = (vertice_points[r].vec[i][0] + tau*vertice_points[r].vec[i][1]) / (2.0*num) ;
			q_norm += pow(q_v[i], 2) ;
		}
		q_norm = sqrt(q_norm) ;
		
		for (i = 0 ; i < 4 ; i++)
			q_v[i] /= q_norm ;
		
		rf = ct ;
		quat[rf*5 + 0] = q_v[0] ;
		quat[rf*5 + 1] = q_v[1] ;
		quat[rf*5 + 2] = q_v[2] ;
		quat[rf*5 + 3] = q_v[3] ;
		quat[rf*5 + 4] = vertice_points[r].weight ;
		
		ct += 1 ;
	}
	if (ct != NUM_VERT / 2) {
		fprintf(stderr, "Inconsistent number of quaternions on vertices!!\n") ;
		return ;
	}

	// select half of the quaternions on edges
	ct = 0 ;
	for (r = 0 ; r < num_edge_point ; r++) {
		flag = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			for (j = 0 ; j < 2; j++) {
				if (edge_points[r].vec[i][j] == 0)
					continue ;
				else if (edge_points[r].vec[i][j] > 0) {
					flag = 1 ;
					break ;
				}
				else{
					flag = -1 ;
					break ;
				}
			}
			
			if (flag != 0)
				break ;
		}
		
		if (flag != 1)
			continue ;
		
		q_norm = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			q_v[i] = (edge_points[r].vec[i][0] + tau*edge_points[r].vec[i][1]) / (2.0*num) ;
			q_norm += pow(q_v[i], 2) ;
		}
		q_norm = sqrt(q_norm) ;
		
		for (i = 0 ; i < 4 ; i++)
			q_v[i] /= q_norm ;
		
		rf = ct + NUM_VERT/2 ;
		quat[rf*5 + 0] = q_v[0] ;
		quat[rf*5 + 1] = q_v[1] ;
		quat[rf*5 + 2] = q_v[2] ;
		quat[rf*5 + 3] = q_v[3] ;
		quat[rf*5 + 4] = edge_points[r].weight ;
		
		ct += 1 ;
	}
	if (ct != num_edge_point / 2) {
		fprintf(stderr, "Inconsistent number of quaternions on edges!!\n") ;
		return ;
	}

	// select half of the quaternions on faces
	ct = 0 ;
	for (r = 0 ; r < num_face_point ; r++) {
		flag = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			for (j = 0 ; j < 2; j++) {
				if (face_points[r].vec[i][j] == 0)
					continue ;
				else if (face_points[r].vec[i][j] > 0) {
					flag = 1 ;
					break ;
				}
				else{
					flag = -1 ;
					break ;
				}
			}
			
			if (flag != 0)
				break ;
		}
		
		if (flag != 1)
			continue ;
		
		q_norm = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			q_v[i] = (face_points[r].vec[i][0] + tau*face_points[r].vec[i][1]) / (2.0*num) ;
			q_norm += pow(q_v[i], 2) ;
		}
		q_norm = sqrt(q_norm) ;
		
		for (i = 0 ; i < 4 ; i++)
			q_v[i] /= q_norm ;
		
		rf = ct + NUM_VERT/2 + num_edge_point/2 ;
		quat[rf*5 + 0] = q_v[0] ;
		quat[rf*5 + 1] = q_v[1] ;
		quat[rf*5 + 2] = q_v[2] ;
		quat[rf*5 + 3] = q_v[3] ;
		quat[rf*5 + 4] = face_points[r].weight ;
		
		ct += 1 ;
	}
	if (ct != num_face_point / 2) {
		fprintf(stderr, "Inconsistent number of quaternions on faces!!\n") ;
		return ;
	}

	// select half of the quaternions on cells
	ct = 0 ;
	for (r = 0 ; r < num_cell_point ; r++) {
		flag = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			for (j = 0 ; j < 2; j++) {
				if (cell_points[r].vec[i][j] == 0)
					continue ;
				else if (cell_points[r].vec[i][j] > 0) {
					flag = 1 ;
					break ;
				}
				else{
					flag = -1 ;
					break ;
				}
			}
			
			if (flag != 0)
				break ;
		}
		
		if (flag != 1)
			continue ;
		
		q_norm = 0 ;
		for (i = 0 ; i < 4 ; i++) {
			q_v[i] = (cell_points[r].vec[i][0] + tau*cell_points[r].vec[i][1]) / (2.0*num) ;
			q_norm += pow(q_v[i], 2) ;
		}
		q_norm = sqrt(q_norm) ;
		
		for (i = 0 ; i < 4 ; i++)
			q_v[i] /= q_norm ;
		
		rf = ct + NUM_VERT/2 + num_edge_point/2 + num_face_point/2 ;
		quat[rf*5 + 0] = q_v[0] ;
		quat[rf*5 + 1] = q_v[1] ;
		quat[rf*5 + 2] = q_v[2] ;
		quat[rf*5 + 3] = q_v[3] ;
		quat[rf*5 + 4] = cell_points[r].weight ;
		
		ct += 1 ;
	}
	if (ct != num_cell_point / 2) {
		fprintf(stderr, "Inconsistent number of quaternions on cells!!\n") ;
		return ;
	}
}

static int reduce_icosahedral(int n, double *quat) {
	int r, t, i, vnum = 8 ;
	int old_num_rot = 10*(5*n*n*n + n), num_rot = 0 ;
	double dist ;
	
	// For all non-vertex quaternions
	for (r = 60 ; r < old_num_rot ; ++r) {
		int keep_quat = 1 ;
		
		// Calculate distance to quat[8]
		// which should be {1,0,0,0}
		double dist0 = 0. ;
		for (t = 0 ; t < 4 ; ++t)
			dist0 += quat[r*5+t] * quat[vnum*5+t] ;
		dist0 = 1. - dist0*dist0 ;
		
		// Calculate distance to all other vertex quaternions
		for (i = 0 ; i < 60 ; ++i) {
			if (i == vnum)
				continue ;
			
			dist = 0 ;
			for (t = 0 ; t < 4 ; ++t)
				dist += quat[r*5+t] * quat[i*5+t] ;
			dist = 1. - dist*dist ;
			
			if (dist < dist0) {
				keep_quat = 0 ;
				break ;
			}
		}
		
		// If closest vertex is 8, keep quaternion
		if (keep_quat) {
			for (t = 0 ; t < 5 ; ++t)
				quat[(60+num_rot)*5+t] = quat[r*5+t] ;
			num_rot++ ;
		}
	}
	
	// Move kept quaternions to first indices
	for (t = 0 ; t < 5 ; ++t)
		quat[0*5+t] = quat[8*5+t] ;
	
	for (r = 0 ; r < num_rot ; ++r)
	for (t = 0 ; t < 5 ; ++t)
		quat[(1+r)*5+t] = quat[(60+r)*5+t] ;
	num_rot++ ;
	
	fprintf(stderr, "num_rot = %d -> %d\n", old_num_rot, num_rot) ;
	return num_rot ;
}

static void quat_free_mem(int num) {
	free(vertice_points) ;
	
	if (num > 1)
		free(edge_points) ;
	if (num > 2)
		free(face_points) ;
	if (num > 3)
		free(cell_points) ;
}

// Public functions below

int quat_gen(struct rotation *quat, int num_div, double sigma) {
	int r, num_rel = 0 ;
	double angle, min_dist2, total_weight = 0. ;
	const double thresh = 1.e-2 ;
	
	make_vertex(num_div) ; 
	min_dist2 = calc_min_dist2() ;
	make_edge(num_div, min_dist2) ;
	make_face(num_div, min_dist2) ; 
	make_cell(num_div, min_dist2) ;
	make_map(min_dist2) ;
	
	quat_setup(num_div, &quat->quat, &quat->num_rot) ;
	
	if (num_div > 1)
		refine_edge(num_div) ;
	if (num_div > 2)
		refine_face(num_div) ;
	if (num_div > 3)
		refine_cell(num_div) ;
	
	print_quat(num_div, quat->quat) ;
	
	if (quat->icosahedral_flag)
		quat->num_rot = reduce_icosahedral(num_div, quat->quat) ;
	
	quat_free_mem(num_div) ;
	
	for (r = 0 ; r < quat->num_rot ; ++r) {
		// Angle w.r.t (1,0,0,0) quaternion
		angle = acos(2*pow(quat->quat[r*5+0], 2.) - 1) ;
		// Gaussian weight as a function of angle
		quat->quat[r*5 + 4] = exp(-0.5*pow(angle/sigma, 2.f)) ;
	}
	
	for (r = 0 ; r < quat->num_rot ; ++r)
	if (quat->quat[r*5 + 4] > thresh) {
		num_rel++ ;
		total_weight += quat->quat[r*5 + 4] ;
	}
	else {
		quat->quat[r*5 + 4] = -1. ;
	}
	
	for (r = 0 ; r < quat->num_rot ; ++r)
	if (quat->quat[r*5 + 4] > thresh)
		quat->quat[r*5 + 4] /= total_weight ;
	
	fprintf(stderr, "Number of relevant orientations = %d/%d\n", num_rel, quat->num_rot) ;
	
	return quat->num_rot ;
}

int quat_parse(struct rotation *quat, char *fname) {
	int r, t ;
	
	FILE *fp = fopen(fname, "r") ;
	if (fp == NULL) {
		fprintf(stderr, "quaternion file %s not found. Exiting.\n", fname) ;
		return -1 ;
	}
	double total_weight = 0. ;
	fscanf(fp, "%d", &quat->num_rot) ;
	quat->quat = malloc(quat->num_rot * 5 * sizeof(double)) ;
	for (r = 0 ; r < quat->num_rot ; ++r) {
		for (t = 0 ; t < 5 ; ++t)
			fscanf(fp, "%lf", &quat->quat[r*5 + t]) ;
		total_weight += quat->quat[r*5 + 4] ;
	}
	total_weight = 1. / total_weight ;
	for (r = 0 ; r < quat->num_rot ; ++r)
		quat->quat[r*5 + 4] *= total_weight ;
	fclose(fp) ;
	
	return quat->num_rot ;
}

void quat_divide(struct rotation *quat, int rank, int num_proc) {
	quat->num_rot_p = quat->num_rot / num_proc ;
	if (rank < (quat->num_rot % num_proc))
		quat->num_rot_p++ ;
	if (num_proc > 1) {
		char hname[99] ;
		gethostname(hname, 99) ;
		fprintf(stderr, "%d: %s: num_rot_p = %d\n", rank, hname, quat->num_rot_p) ;
	}
}

void quat_free(struct rotation *quat) {
	if (quat == NULL)
		return ;
	
	free(quat->quat) ;
	free(quat) ;
}

