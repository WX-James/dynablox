#ifndef LIDAR_MOTION_DETECTION_CLUSTERING_H_
#define LIDAR_MOTION_DETECTION_CLUSTERING_H_

#include <memory>
#include <vector>

#include <pcl/pcl_base.h>
#include <pcl/search/flann_search.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/impl/transforms.hpp>
#include <pcl_ros/point_cloud.h>
#include <voxblox/core/block_hash.h>
#include <voxblox/core/layer.h>
#include <voxblox/integrator/integrator_utils.h>
#include <voxblox/utils/approx_hash_array.h>
#include <voxblox/utils/timing.h>

#include "lidar_motion_detection/3rd_party/config_utilities.hpp"
#include "lidar_motion_detection/common/types.h"

namespace motion_detection {

class Clustering {
 public:
  // Config.
  struct Config : public config_utilities::Config<Config> {
    // Filter out smaller or larger clusters than specified.
    int min_cluster_size = 20;
    int max_cluster_size = 20000;

    Config() { setConfigName("Clustering"); }

   protected:
    void setupParamsAndPrinting() override;
    void checkParams() const override;
  };

  // Constructor.
  Clustering(const Config& config,
             voxblox::Layer<voxblox::TsdfVoxel>::Ptr tsdf_layer);

  // Types.
  using ClusterIndices = std::vector<voxblox::VoxelKey>;

  /**
   * @brief Execute all clustering steps to identify the final clusters. Also
   * modifies the infos?
   *
   * @param block2index_hash
   * @param blockwise_voxel2point_map
   * @param occupied_ever_free_voxel_indices
   * @param cloud
   * @param cloud_info
   * @param frame_counter
   * @return Clusters
   */
  Clusters performClustering(
      voxblox::AnyIndexHashMapType<int>::type& block2index_hash,
      std::vector<voxblox::HierarchicalIndexIntMap>& blockwise_voxel2point_map,
      ClusterIndices& occupied_ever_free_voxel_indices, const Cloud& cloud,
      CloudInfo& cloud_info, int frame_counter) const;

  /**
   * @brief Cluster all currently occupied voxels that are next to an ever-free
   * voxel.
   *
   * @param occupied_ever_free_voxel_indices Occupied ever-free voxel indices to
   * seed the clusters.
   * @param frame_counter Frame number to verify added voxels contain points
   * this scan.
   * @return Vector of all found clusters.
   */
  std::vector<ClusterIndices> voxelClustering(
      const ClusterIndices& occupied_ever_free_voxel_indices,
      int frame_counter) const;

  /**
   * @brief Grow a single cluster from a seed voxel key. All voxels that are not
   * yet processed are added to the cluster and labeled as processed and
   * dynamic. Only ever-free voxels can further grow the cluster.
   *
   * @param seed Voxel key to start clustering from.
   * @param frame_counter Frame number to verify added voxels contain points
   * this scan.
   * @return Voxel keys of all voxels of the cluster.
   */
  ClusterIndices growCluster(const voxblox::VoxelKey& seed,
                             int frame_counter) const;

  /**
   * @brief Use the voxel level clustering to assign all points to clusters.
   *
   * @param block2points_map Mapping of blocks to points in the cloud.
   * @param blockwise_voxel_map Mapping of blocks to voxels containing points.
   * @param cloud The complete scan point cloud.
   * @param voxel_cluster_ind Voxel indices per cluster.
   * @return All clusters.
   */
  Clusters inducePointClusters(
      const voxblox::AnyIndexHashMapType<int>::type& block2points_map,
      const std::vector<voxblox::HierarchicalIndexIntMap>& blockwise_voxel_map,
      const Cloud& cloud,
      const std::vector<ClusterIndices>& voxel_cluster_ind) const;

  /**
   * @brief Removes all clusters that don't meet the filtering criteria.
   *
   * @param candidates list of clusters that will be filtered.
   */
  void applyClusterLevelFilters(Clusters& candidates) const;

  /**
   * @brief Sets dynamic flag on point level (includes points belonging to
   * extension of high confidence detection clusters)
   *
   * @param clusters Clusters whose points will be labeled.
   * @param cloud_info Cloud info where the label is placed.
   */
  void setClusterLevelDynamicFlagOfallPoints(const Clusters& clusters,
                                             CloudInfo& cloud_info) const;

 private:
  const Config config_;
  voxblox::Layer<voxblox::TsdfVoxel>::Ptr tsdf_layer_;
};

}  // namespace motion_detection

#endif  // LIDAR_MOTION_DETECTION_CLUSTERING_H_
