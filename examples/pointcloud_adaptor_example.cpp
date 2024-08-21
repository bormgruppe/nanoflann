/***********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright 2011-2024 Jose Luis Blanco (joseluisblancoc@gmail.com).
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************/

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <nanoflann.hpp>

// Declare custom container PointCloud<T>:
#include "utils.h"

void dump_mem_usage();

// And this is the "dataset to kd-tree" adaptor class:
template <typename Derived>
struct PointCloudAdaptor
{
    using coord_t = typename Derived::coord_t;

    struct PointAdaptor
    {
        const typename Derived::PointType& point;

        inline coord_t get_component(size_t dim) const
        {
            return point.get_component(dim);
        }

        inline coord_t get_signed_distance(size_t dim, coord_t val) const
        {
            return point.get_signed_distance(dim, val);
        }

        inline coord_t get_distance_to(const PointAdaptor& pt) const
        {
            return point.get_distance_to(pt.point);
        }
    };
    using PointType = PointAdaptor;

    const Derived& obj;  //!< A const ref to the data set origin

    /// The constructor that sets the data set source
    PointCloudAdaptor(const Derived& obj_) : obj(obj_) {}

    /// CRTP helper method
    inline const Derived& derived() const { return obj; }

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const
    {
        return derived().pts.size();
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is inlined and the "dim" argument is typically an immediate
    // value, the
    //  "if/else's" are actually solved at compile time.
    inline PointAdaptor kdtree_get_pt(const size_t idx) const
    {
        return PointAdaptor{derived().pts[idx]};
    }

    // Get limits for list of points
    inline void kdtree_get_limits(
        const uint32_t* ix, size_t count, const size_t dim, coord_t& limit_min,
        coord_t& limit_max) const
    {
        limit_min = limit_max = kdtree_get_pt(ix[0]).get_component(dim);
        for (size_t k = 1; k < count; ++k)
        {
            const coord_t value = kdtree_get_pt(ix[k]).get_component(dim);
            if (value < limit_min) limit_min = value;
            if (value > limit_max) limit_max = value;
        }
    }

};  // end of PointCloudAdaptor

template <typename num_t>
void kdtree_demo(const size_t N)
{
    PointCloud<num_t> cloud;

    // Generate points:
    generateRandomPointCloud(cloud, N);

    using PC2KD = PointCloudAdaptor<PointCloud<num_t>>;
    const PC2KD pc2kd(cloud);  // The adaptor

    // construct a kd-tree index:
    using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<num_t, PC2KD>, PC2KD, 3 /* dim */
        >;

    dump_mem_usage();

    auto do_knn_search = [](const my_kd_tree_t& index) {
        // do a knn search
        const size_t                   num_results = 1;
        size_t                         ret_index;
        num_t                          out_dist_sqr;
        nanoflann::KNNResultSet<num_t> resultSet(num_results);
        const PointCloud<num_t>::Point query_pt{0.5, 0.5, 0.5};

        resultSet.init(&ret_index, &out_dist_sqr);
        index.findNeighbors(resultSet, {query_pt});

        std::cout << "knnSearch(nn=" << num_results << "): \n";
        std::cout << "ret_index=" << ret_index
                  << " out_dist_sqr=" << out_dist_sqr << std::endl;
    };

    my_kd_tree_t index1(3 /*dim*/, pc2kd, {10 /* max leaf */});
    my_kd_tree_t index2(3 /*dim*/, pc2kd);

    dump_mem_usage();

    do_knn_search(index1);
    do_knn_search(index2);
}

int main()
{
    // Randomize Seed
    srand((unsigned int)time(NULL));
    kdtree_demo<float>(1000000);
    kdtree_demo<double>(1000000);
    return 0;
}
