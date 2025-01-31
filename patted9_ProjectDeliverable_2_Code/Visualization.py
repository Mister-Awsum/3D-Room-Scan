import numpy as np
import open3d as o3d

if __name__ == "__main__":
    total_scans = 3
    total_steps = 16
    pcd = o3d.io.read_point_cloud("points.xyz", format="xyz")

    #Numerical representation      
    print("The PCD array:")
    print(np.asarray(pcd.points))

    #Graphical representation     
    print("Visualize the PCD: ")
    o3d.visualization.draw_geometries([pcd])

    
    #Unique number for vertices
    yz_slice_vertex = []
    for x in range(0,total_scans*total_steps):
        yz_slice_vertex.append([x])

    #Define coordinates to connect lines in each yz slice        
    lines = []  
    for x in range(0, total_scans*total_steps, total_steps):
        for i in range(total_steps):
            if i == (total_steps-1):
                lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x]])
            else:
                lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x+i+1]])
            

    #Define coordinates to connect lines between current and next yz slice        
    for x in range(0, (total_scans * total_steps - total_steps - 1), total_steps):
        for i in range(total_steps):
            lines.append([yz_slice_vertex[x+i], yz_slice_vertex[x+i+total_steps]])

    #This line maps the lines to the 3d coordinate vertices
    line_set = o3d.geometry.LineSet(points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),lines=o3d.utility.Vector2iVector(lines))

    #Lets see what our point cloud data with lines looks like graphically       
    o3d.visualization.draw_geometries([line_set])