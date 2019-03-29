#include "target_manager/TargetManager.h"

TargetManager::TargetManager(){}

void TargetManager::init(ros::NodeHandle nh){

    // paramter parsing for option 
    nh.param<string>("world_frame_id",world_frame_id,"/world");
    nh.param<string>("target_frame_id",target_frame_id,"/target");
    nh.param<double>("safe_radius",traj_option.safe_r,0.2);
    nh.param<int>("N_safe_pnts",traj_option.N_safe_pnts,2);
    nh.param<int>("objective_derivative",traj_option.objective_derivative,3);
    nh.param<int>("poly_order",traj_option.poly_order,6);
    nh.param<double>("weights_deviation",traj_option.w_d,0.005);

    // register 
    pub_marker_waypoints = nh.advertise<visualization_msgs::MarkerArray>("target_waypoints",1);
    sub_waypoints = nh.subscribe("/target_waypoints",1,&TargetManager::callback_waypoint,this);
    pub_path = nh.advertise<nav_msgs::Path>("target_global_path",1);
    br_ptr = new tf::TransformBroadcaster();
}

void TargetManager::callback_waypoint(const geometry_msgs::PoseStampedConstPtr & pose){
    if (is_insert_permit){
        ROS_INFO("point received");
        queue.push_back(*pose);
        visualization_msgs::Marker marker;
        
        marker.action = 0;
        marker.header.frame_id = world_frame_id;
        marker.type = visualization_msgs::Marker::CUBE;
        marker.pose = pose->pose;
        float scale = 0.1;
        marker.scale.x = scale;
        marker.scale.y = scale;
        marker.scale.z = scale;
        marker.color.r = 1;
        marker.color.a = 1;
        marker.id = queue.size();
        wpnt_markerArray.markers.push_back(marker);
        
        // lets print in board  
        string line = "[Target manager] recieved point: " 
                        + to_string(pose->pose.position.x) + " , "
                        + to_string(pose->pose.position.y);
        
        ROS_INFO(line.c_str());        
    }else{
         
 
        ROS_WARN("[Target manager] insertion not allowed");
        // std::cout<<"insertion not allowed"<<std::endl;
    }
}


bool TargetManager::global_path_generate(double tf){

    nav_msgs::Path waypoints;

    waypoints.poses = queue;
    TimeSeries knots(queue.size());
    knots.setLinSpaced(queue.size(),0,tf);
    planner.path_gen(knots,waypoints,geometry_msgs::Twist(),geometry_msgs::Twist(),traj_option); 
    if(planner.is_spline_valid()){
        is_path = true;
        global_path = planner.get_path();
        global_path.header.frame_id  = world_frame_id;
        ROS_INFO("[Target manager] global path obtained.");    
        return true;
    }
    else{
        ROS_INFO("[Target manager] path generatoin failed.");   
        return false; 
    }
}

void TargetManager::session(double t_eval){

    pub_marker_waypoints.publish(wpnt_markerArray);
    if(is_path){
        pub_path.publish(global_path);
        broadcast_target_tf(t_eval);
    }
}

void TargetManager::clear_waypoint(){
    queue.clear();
    wpnt_markerArray.markers.clear();
    ROS_INFO("[Target manager] queue cleared ");    
}

void TargetManager::pop_waypoint(){
    queue.pop_back();
    wpnt_markerArray.markers.pop_back();
    ROS_INFO("[Target manager] queue pop ");    
}

void TargetManager::broadcast_target_tf(double t_eval){    

    tf::TransformBroadcaster br; 

    Point eval_point = planner.point_eval_spline(t_eval);
    // Twist eval_vel = planner.vel_eval_spline(t_eval);

    tf::Transform transform;
    // float target_yaw = atan2(eval_vel.linear.y,eval_vel.linear.x);
    tf::Quaternion q;
    q.setRPY(0, 0, 0);

    transform.setOrigin(tf::Vector3(eval_point.x,eval_point.y,eval_point.z));
    transform.setRotation(q);
    br_ptr->sendTransform(tf::StampedTransform(transform,ros::Time::now(),world_frame_id,target_frame_id));
}

void TargetManager::queue_file_load(vector<geometry_msgs::PoseStamped>& wpnt_replace){

    this->queue = wpnt_replace;
    wpnt_markerArray.markers.clear();

    
    for(auto it = wpnt_replace.begin();it<wpnt_replace.end();it++){
        
        visualization_msgs::Marker marker;

        marker.action = 0;
        marker.header.frame_id  = world_frame_id;
        marker.type = visualization_msgs::Marker::CUBE;
        marker.pose = it->pose;
        
        std::cout<< it->pose.position.x <<" , "<< it->pose.position.y <<" , "<<it->pose.position.z<<std::endl;

        float scale = 0.1;
        marker.scale.x = scale;
        marker.scale.y = scale;
        marker.scale.z = scale;
        marker.color.r = 1;
        marker.color.b = 1;

        marker.color.a = 1;
        marker.id = wpnt_markerArray.markers.size();
        wpnt_markerArray.markers.push_back(marker);
        
    }

}