/*
 * Copyright (c) 2017, Miroslav Stoyanov
 *
 * This file is part of
 * Toolkit for Adaptive Stochastic Modeling And Non-Intrusive ApproximatioN: TASMANIAN
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * UT-BATTELLE, LLC AND THE UNITED STATES GOVERNMENT MAKE NO REPRESENTATIONS AND DISCLAIM ALL WARRANTIES, BOTH EXPRESSED AND IMPLIED.
 * THERE ARE NO EXPRESS OR IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, OR THAT THE USE OF THE SOFTWARE WILL NOT INFRINGE ANY PATENT,
 * COPYRIGHT, TRADEMARK, OR OTHER PROPRIETARY RIGHTS, OR THAT THE SOFTWARE WILL ACCOMPLISH THE INTENDED RESULTS OR THAT THE SOFTWARE OR ITS USE WILL NOT RESULT IN INJURY OR DAMAGE.
 * THE USER ASSUMES RESPONSIBILITY FOR ALL LIABILITIES, PENALTIES, FINES, CLAIMS, CAUSES OF ACTION, AND COSTS AND EXPENSES, CAUSED BY, RESULTING FROM OR ARISING OUT OF,
 * IN WHOLE OR IN PART THE USE, STORAGE OR DISPOSAL OF THE SOFTWARE.
 */

#ifndef __TASMANIAN_SPARSE_GRID_GLOBAL_CPP
#define __TASMANIAN_SPARSE_GRID_GLOBAL_CPP

#include "tsgGridGlobal.hpp"

#include "tsgHiddenExternals.hpp"
#include "tsgUtils.hpp"

namespace TasGrid{

GridGlobal::GridGlobal() : alpha(0.0), beta(0.0){}
GridGlobal::~GridGlobal(){}

template<bool useAscii> void GridGlobal::write(std::ostream &os) const{
    if (useAscii){ os << std::scientific; os.precision(17); }
    IO::writeNumbers<useAscii, IO::pad_rspace>(os, num_dimensions, num_outputs);
    IO::writeNumbers<useAscii, IO::pad_line>(os, alpha, beta);
    IO::writeRule<useAscii>(rule, os);
    if (rule == rule_customtabulated)
        custom.write<useAscii>(os);

    tensors.write<useAscii>(os);
    active_tensors.write<useAscii>(os);
    if (!active_w.empty())
        IO::writeVector<useAscii, IO::pad_line>(active_w, os);

    IO::writeFlag<useAscii, IO::pad_auto>(!points.empty(), os);
    if (!points.empty()) points.write<useAscii>(os);
    IO::writeFlag<useAscii, IO::pad_auto>(!needed.empty(), os);
    if (!needed.empty()) needed.write<useAscii>(os);

    IO::writeVector<useAscii, IO::pad_line>(max_levels, os);

    if (num_outputs > 0) values.write<useAscii>(os);

    IO::writeFlag<useAscii, IO::pad_line>(!updated_tensors.empty(), os);
    if (!updated_tensors.empty()){
        updated_tensors.write<useAscii>(os);
        updated_active_tensors.write<useAscii>(os);
        IO::writeVector<useAscii, IO::pad_line>(updated_active_w, os);
    }
}

template<bool useAscii> void GridGlobal::read(std::ifstream &is){
    reset(true); // true deletes any custom rule
    num_dimensions = IO::readNumber<useAscii, int>(is);
    num_outputs = IO::readNumber<useAscii, int>(is);
    alpha = IO::readNumber<useAscii, double>(is);
    beta = IO::readNumber<useAscii, double>(is);
    rule = IO::readRule<useAscii>(is);
    if (rule == rule_customtabulated) custom.read<useAscii>(is);
    tensors.read<useAscii>(is);
    active_tensors.read<useAscii>(is);
    active_w.resize((size_t) active_tensors.getNumIndexes());
    IO::readVector<useAscii>(is, active_w);

    if (IO::readFlag<useAscii>(is)) points.read<useAscii>(is);
    if (IO::readFlag<useAscii>(is)) needed.read<useAscii>(is);

    max_levels.resize((size_t) num_dimensions);
    IO::readVector<useAscii>(is, max_levels);

    if (num_outputs > 0) values.read<useAscii>(is);

    int oned_max_level;
    if (IO::readFlag<useAscii>(is)){
        updated_tensors.read<useAscii>(is);
        oned_max_level = updated_tensors.getMaxIndex();

        updated_active_tensors.read<useAscii>(is);

        updated_active_w.resize((size_t) updated_active_tensors.getNumIndexes());
        IO::readVector<useAscii>(is, updated_active_w);
    }else{
        oned_max_level = *std::max_element(max_levels.begin(), max_levels.end());
    }

    wrapper.load(custom, oned_max_level, rule, alpha, beta);

    recomputeTensorRefs((points.empty()) ? needed : points);
}

template void GridGlobal::write<true>(std::ostream &) const;
template void GridGlobal::write<false>(std::ostream &) const;
template void GridGlobal::read<true>(std::ifstream &);
template void GridGlobal::read<false>(std::ifstream &);

void GridGlobal::reset(bool includeCustom){
    clearAccelerationData();
    tensor_refs = std::vector<std::vector<int>>();
    wrapper = OneDimensionalWrapper();
    tensors = MultiIndexSet();
    active_tensors = MultiIndexSet();
    active_w = std::vector<int>();
    points = MultiIndexSet();
    needed = MultiIndexSet();
    values = StorageSet();
    updated_tensors = MultiIndexSet();
    updated_active_tensors = MultiIndexSet();
    updated_active_w = std::vector<int>();
    if (includeCustom) custom = CustomTabulated();
    num_dimensions = num_outputs = 0;
}

void GridGlobal::clearRefinement(){
    needed = MultiIndexSet();
    updated_tensors = MultiIndexSet();
    updated_active_tensors = MultiIndexSet();
    updated_active_w = std::vector<int>();
}

MultiIndexSet GridGlobal::selectTensors(size_t dims, int depth, TypeDepth type, const std::vector<int> &anisotropic_weights,
                                        TypeOneDRule crule, std::vector<int> const &level_limits) const{
    if (OneDimensionalMeta::isExactLevel(type)){ // no need to know exactness
        return MultiIndexManipulations::selectTensors(dims, depth, type, [&](int l) -> int{ return l; },
                                                      anisotropic_weights, level_limits);
    }else{ // work with exactness specific to the rule
        if (crule == rule_customtabulated){
            if (OneDimensionalMeta::isExactQuadrature(type)){
                return MultiIndexManipulations::selectTensors(dims, depth, type, [&](int l) -> int{ return custom.getQExact(l); },
                                                              anisotropic_weights, level_limits);
            }else{
                return MultiIndexManipulations::selectTensors(dims, depth, type, [&](int l) -> int{ return custom.getIExact(l); },
                                                              anisotropic_weights, level_limits);
            }
        }else{ // using regular OneDimensionalMeta
            if (OneDimensionalMeta::isExactQuadrature(type)){
                return MultiIndexManipulations::selectTensors(dims, depth, type, [&](int l) -> int{ return OneDimensionalMeta::getQExact(l, crule); },
                                                              anisotropic_weights, level_limits);
            }else{
                return MultiIndexManipulations::selectTensors(dims, depth, type, [&](int l) -> int{ return OneDimensionalMeta::getIExact(l, crule); },
                                                              anisotropic_weights, level_limits);
            }
        }
    }
}
void GridGlobal::recomputeTensorRefs(const MultiIndexSet &work){
    int nz_weights = active_tensors.getNumIndexes();
    tensor_refs.resize((size_t) nz_weights);
    if (OneDimensionalMeta::isNonNested(rule)){
        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<nz_weights; i++)
            MultiIndexManipulations::referencePoints<false>(active_tensors.getIndex(i), wrapper, work, tensor_refs[i]);
    }else{
        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<nz_weights; i++)
            MultiIndexManipulations::referencePoints<true>(active_tensors.getIndex(i), wrapper, work, tensor_refs[i]);
    }
}

void GridGlobal::makeGrid(int cnum_dimensions, int cnum_outputs, int depth, TypeDepth type, TypeOneDRule crule, const std::vector<int> &anisotropic_weights, double calpha, double cbeta, const char* custom_filename, const std::vector<int> &level_limits){
    if (crule == rule_customtabulated){
        custom.read(custom_filename);
    }

    MultiIndexSet tset = selectTensors((size_t) cnum_dimensions, depth, type, anisotropic_weights, crule, level_limits);

    setTensors(tset, cnum_outputs, crule, calpha, cbeta);
}
void GridGlobal::copyGrid(const GridGlobal *global){
    custom = CustomTabulated();
    if (global->rule == rule_customtabulated) custom = global->custom;

    MultiIndexSet tset = global->tensors;
    setTensors(tset, global->num_outputs, global->rule, global->alpha, global->beta);

    if ((num_outputs > 0) && (!(global->points.empty()))){ // if there are values inside the source object
        loadNeededPoints(global->values.getValues(0));
    }

    // if there is an active refinement awaiting values
    if (!(global->updated_tensors.empty())){
        updated_tensors = global->updated_tensors;
        updated_active_tensors = global->updated_active_tensors;
        updated_active_w = global->updated_active_w;

        needed = global->needed;

        wrapper.load(custom, global->wrapper.getNumLevels(), rule, alpha, beta);
    }
}

void GridGlobal::setTensors(MultiIndexSet &tset, int cnum_outputs, TypeOneDRule crule, double calpha, double cbeta){
    reset(false);
    num_dimensions = (int) tset.getNumDimensions();
    num_outputs = cnum_outputs;
    rule = crule;
    alpha = calpha;  beta = cbeta;

    tensors = std::move(tset);

    max_levels = MultiIndexManipulations::getMaxIndexes(tensors);

    wrapper.load(custom, *std::max_element(max_levels.begin(), max_levels.end()), rule, alpha, beta);

    std::vector<int> tensors_w = MultiIndexManipulations::computeTensorWeights(tensors);
    active_tensors = MultiIndexManipulations::createActiveTensors(tensors, tensors_w);

    active_w.reserve(active_tensors.getNumIndexes());
    for(auto w : tensors_w) if (w != 0) active_w.push_back(w);

    if (OneDimensionalMeta::isNonNested(rule)){
        needed = MultiIndexManipulations::generateNonNestedPoints(active_tensors, wrapper);
    }else{
        needed = MultiIndexManipulations::generateNestedPoints(tensors, [&](int l) -> int{ return wrapper.getNumPoints(l); });
    }

    recomputeTensorRefs(needed);

    if (num_outputs == 0){
        points = std::move(needed);
        needed = MultiIndexSet();
    }else{
        values.resize(num_outputs, needed.getNumIndexes());
    }
}

void GridGlobal::proposeUpdatedTensors(){
    int max_level = updated_tensors.getMaxIndex();
    wrapper.load(custom, max_level, rule, alpha, beta);

    std::vector<int> updates_tensor_w = MultiIndexManipulations::computeTensorWeights(updated_tensors);
    updated_active_tensors = MultiIndexManipulations::createActiveTensors(updated_tensors, updates_tensor_w);

    updated_active_w.reserve(updated_active_tensors.getNumIndexes());
    for(auto w : updates_tensor_w) if (w != 0) updated_active_w.push_back(w);

    MultiIndexSet new_points = (OneDimensionalMeta::isNonNested(rule)) ?
            MultiIndexManipulations::generateNonNestedPoints(updated_active_tensors, wrapper) :
            MultiIndexManipulations::generateNestedPoints(updated_tensors, [&](int l) -> int{ return wrapper.getNumPoints(l); });

    needed = new_points.diffSets(points);
}

void GridGlobal::updateGrid(int depth, TypeDepth type, const std::vector<int> &anisotropic_weights, const std::vector<int> &level_limits){
    if ((num_outputs == 0) || points.empty()){
        makeGrid(num_dimensions, num_outputs, depth, type, rule, anisotropic_weights, alpha, beta, 0, level_limits);
    }else{
        clearRefinement();

        updated_tensors = selectTensors((size_t) num_dimensions, depth, type, anisotropic_weights, rule, level_limits);

        MultiIndexSet new_tensors = updated_tensors.diffSets(tensors);

        if (!new_tensors.empty()){
            updated_tensors.addMultiIndexSet(tensors);
            proposeUpdatedTensors();
        }
    }
}

void GridGlobal::mapIndexesToNodes(const std::vector<int> &indexes, double *x) const{
    std::transform(indexes.begin(), indexes.end(), x, [&](int i)->double{ return wrapper.getNode(i); });
}

void GridGlobal::getLoadedPoints(double *x) const{
    mapIndexesToNodes(points.getVector(), x);
}
void GridGlobal::getNeededPoints(double *x) const{
    mapIndexesToNodes(needed.getVector(), x);
}
void GridGlobal::getPoints(double *x) const{
    if (points.empty()){ getNeededPoints(x); }else{ getLoadedPoints(x); };
}

void GridGlobal::getQuadratureWeights(double weights[]) const{
    std::fill_n(weights, (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes(), 0.0);

    std::vector<int> num_oned_points(num_dimensions);
    for(int n=0; n<active_tensors.getNumIndexes(); n++){
        const int* levels = active_tensors.getIndex(n);
        num_oned_points[0] = wrapper.getNumPoints(levels[0]);
        int num_tensor_points = num_oned_points[0];
        for(int j=1; j<num_dimensions; j++){
            num_oned_points[j] = wrapper.getNumPoints(levels[j]);
            num_tensor_points *= num_oned_points[j];
        }
        double tensor_weight = (double) active_w[n];

        #pragma omp parallel for schedule(static)
        for(int i=0; i<num_tensor_points; i++){
            int t = i;
            double w = 1.0;
            for(int j=num_dimensions-1; j>=0; j--){
                w *= wrapper.getWeight(levels[j], t % num_oned_points[j]);
                t /= num_oned_points[j];
            }
            weights[tensor_refs[n][i]] += tensor_weight * w;
        }
    }
}

void GridGlobal::getInterpolationWeights(const double x[], double weights[]) const{
    std::fill_n(weights, (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes(), 0.0);

    CacheLagrange<double> lcache(num_dimensions, max_levels, wrapper, x);

    std::vector<int> num_oned_points(num_dimensions);
    for(int n=0; n<active_tensors.getNumIndexes(); n++){
        const int* levels = active_tensors.getIndex(n);
        num_oned_points[0] = wrapper.getNumPoints(levels[0]);
        int num_tensor_points = num_oned_points[0];
        for(int j=1; j<num_dimensions; j++){
            num_oned_points[j] = wrapper.getNumPoints(levels[j]);
            num_tensor_points *= num_oned_points[j];
        }
        double tensor_weight = (double) active_w[n];
        for(int i=0; i<num_tensor_points; i++){
            int t = i;
            double w = 1.0;
            for(int j=num_dimensions-1; j>=0; j--){
                w *= lcache.getLagrange(j, levels[j], t % num_oned_points[j]);
                t /= num_oned_points[j];
            }
            weights[tensor_refs[n][i]] += tensor_weight * w;
        }
    }
}

void GridGlobal::acceptUpdatedTensors(){
    if (points.empty()){
        points = std::move(needed);
        needed = MultiIndexSet();
    }else if (!needed.empty()){
        points.addMultiIndexSet(needed);
        needed = MultiIndexSet();

        tensors = std::move(updated_tensors);
        updated_tensors = MultiIndexSet();

        active_tensors = std::move(updated_active_tensors);
        updated_active_tensors = MultiIndexSet();

        active_w = std::move(updated_active_w);
        updated_active_w = std::vector<int>();

        max_levels = MultiIndexManipulations::getMaxIndexes(tensors);

        recomputeTensorRefs(points);
    }
}

void GridGlobal::loadNeededPoints(const double *vals){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_values.clear();
    #endif
    if (points.empty() || needed.empty()){
        values.setValues(vals);
    }else{
        values.addValues(points, needed, vals);
    }
    acceptUpdatedTensors();
}
void GridGlobal::mergeRefinement(){
    if (needed.empty()) return; // nothing to do
    int num_all_points = getNumLoaded() + getNumNeeded();
    std::vector<double> vals(((size_t) num_all_points) * ((size_t) num_outputs), 0.0);
    values.setValues(vals);
    acceptUpdatedTensors();
}

void GridGlobal::beginConstruction(){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal(num_dimensions, num_outputs));
    if (points.empty()){ // if we start dynamic construction from an empty grid
        for(int i=0; i<tensors.getNumIndexes(); i++){
            const int *t = tensors.getIndex(i);
            double weight = -1.0 / (1.0 + (double) std::accumulate(t, t + num_dimensions, 0));
            dynamic_values->addTensor(t, [&](int l)->int{ return wrapper.getNumPoints(l); }, weight);
        }
        tensors = MultiIndexSet();
        active_tensors = MultiIndexSet();
        active_w = std::vector<int>();
        needed = MultiIndexSet();
        values.resize(num_outputs, 0);
    }
}
void GridGlobal::writeConstructionDataBinary(std::ofstream &ofs) const{
    dynamic_values->write<false>(ofs);
}
void GridGlobal::writeConstructionData(std::ofstream &ofs) const{
    dynamic_values->write<true>(ofs);
}
void GridGlobal::readConstructionDataBinary(std::ifstream &ifs){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal((size_t) num_dimensions, (size_t) num_outputs));
    dynamic_values->read<false>(ifs);
    int max_level = dynamic_values->getMaxTensor();
    if (max_level + 1 > wrapper.getNumLevels())
        wrapper.load(custom, max_level, rule, alpha, beta);
    dynamic_values->reloadPoints([&](int l)->int{ return wrapper.getNumPoints(l); });
}
void GridGlobal::readConstructionData(std::ifstream &ifs){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal((size_t) num_dimensions, (size_t) num_outputs));
    dynamic_values->read<true>(ifs);
    int max_level = dynamic_values->getMaxTensor();
    if (max_level + 1 > wrapper.getNumLevels())
        wrapper.load(custom, max_level, rule, alpha, beta);
    dynamic_values->reloadPoints([&](int l)->int{ return wrapper.getNumPoints(l); });
}
std::vector<double> GridGlobal::getCandidateConstructionPoints(TypeDepth type, int output, const std::vector<int> &level_limits){
    std::vector<int> weights;
    if ((type == type_iptotal) || (type == type_ipcurved) || (type == type_qptotal) || (type == type_qpcurved)){
        int min_needed_points = ((type == type_ipcurved) || (type == type_qpcurved)) ? 4 * num_dimensions : 2 * num_dimensions;
        if (points.getNumIndexes() > min_needed_points) // if there are enough points to estimate coefficients
            estimateAnisotropicCoefficients(type, output, weights);
    }
    return getCandidateConstructionPoints(type, weights, level_limits);
}
std::vector<double> GridGlobal::getCandidateConstructionPoints(TypeDepth type, const std::vector<int> &anisotropic_weights, const std::vector<int> &level_limits){
    MultiIndexManipulations::ProperWeights weights((size_t) num_dimensions, type, anisotropic_weights);

    // computing the weight for each index requires the cache for the one dimensional indexes
    // the cache for the one dimensional indexes requires the exactness
    // exactness can be one of 5 cases based on level/interpolation/quadrature for custom or known rule
    // the number of required exactness entries is not known until later and we have to be careful not to exceed the levels available for the custom rule
    // thus, we cache the exactness with a delay
    std::vector<int> effective_exactness;
    auto get_exact = [&](int l) -> int{ return effective_exactness[l]; };
    auto build_exactness = [&](size_t num) ->
        void{
            effective_exactness.resize(num);
            if(OneDimensionalMeta::isExactLevel(type)){
                for(size_t i=0; i<num; i++) effective_exactness[i] = (int) i;
            }else if (OneDimensionalMeta::isExactInterpolation(type)){
                if (rule == rule_customtabulated){
                    for(size_t i=0; i<num; i++) effective_exactness[i] = custom.getIExact((int) i);
                }else{
                    for(size_t i=0; i<num; i++) effective_exactness[i] = OneDimensionalMeta::getIExact((int) i, rule);
                }
            }else{ // must be quadrature
                if (rule == rule_customtabulated){
                    for(size_t i=0; i<num; i++) effective_exactness[i] = custom.getQExact((int) i);
                }else{
                    for(size_t i=0; i<num; i++) effective_exactness[i] = OneDimensionalMeta::getQExact((int) i, rule);
                }
            }
        };

    if (weights.contour == type_level){
        std::vector<std::vector<int>> cache;
        return getCandidateConstructionPoints([&](int const *t) -> double{
            if (cache.empty()){
                build_exactness((size_t) wrapper.getNumLevels());
                cache = MultiIndexManipulations::generateLevelWeightsCache<int, type_level, true>(weights, get_exact, wrapper.getNumLevels());
            }

            return (double) MultiIndexManipulations::getIndexWeight<int, type_level>(t, cache);
        }, level_limits);
    }else if (weights.contour == type_curved){
        std::vector<std::vector<double>> cache;
        return getCandidateConstructionPoints([&](int const *t) -> double{
            if (cache.empty()){
                build_exactness((size_t) wrapper.getNumLevels());
                cache = MultiIndexManipulations::generateLevelWeightsCache<double, type_curved, true>(weights, get_exact, wrapper.getNumLevels());
            }

            return MultiIndexManipulations::getIndexWeight<double, type_curved>(t, cache);
        }, level_limits);
    }else{
        std::vector<std::vector<double>> cache;
        return getCandidateConstructionPoints([&](int const *t) -> double{
            if (cache.empty()){
                build_exactness((size_t) wrapper.getNumLevels());
                cache = MultiIndexManipulations::generateLevelWeightsCache<double, type_hyperbolic, true>(weights, get_exact, wrapper.getNumLevels());
            }

            return MultiIndexManipulations::getIndexWeight<double, type_hyperbolic>(t, cache);
        }, level_limits);
    }
}
std::vector<double> GridGlobal::getCandidateConstructionPoints(std::function<double(const int *)> getTensorWeight, const std::vector<int> &level_limits){
    dynamic_values->clearTesnors(); // clear old tensors
    MultiIndexSet init_tensors = dynamic_values->getInitialTensors(); // get the initial tensors (created with make grid)

    MultiIndexSet new_tensors = (level_limits.empty()) ?
        MultiIndexManipulations::addExclusiveChildren<false>(tensors, init_tensors, level_limits) :
        MultiIndexManipulations::addExclusiveChildren<true>(tensors, init_tensors, level_limits);

    if (!new_tensors.empty()){
        auto max_indexes = MultiIndexManipulations::getMaxIndexes(new_tensors);
        int max_level = *std::max_element(max_indexes.begin(), max_indexes.end());
        if (max_level+1 > wrapper.getNumLevels())
            wrapper.load(custom, max_level, rule, alpha, beta);
    }

    std::vector<double> tweights(new_tensors.getNumIndexes());
    for(int i=0; i<new_tensors.getNumIndexes(); i++)
        tweights[i] = (double) getTensorWeight(new_tensors.getIndex(i));

    for(int i=0; i<new_tensors.getNumIndexes(); i++){
        const int *t = new_tensors.getIndex(i);
        dynamic_values->addTensor(t, [&](int l)->int{ return wrapper.getNumPoints(l); }, tweights[i]);
    }
    std::vector<int> node_indexes;
    dynamic_values->getNodesIndexes(node_indexes);
    std::vector<double> x(node_indexes.size());
    mapIndexesToNodes(node_indexes, x.data());
    return x;
}
void GridGlobal::loadConstructedPoint(const double x[], const std::vector<double> &y){
    std::vector<int> p(num_dimensions);
    for(int j=0; j<num_dimensions; j++){
        int i = 0;
        while(fabs(wrapper.getNode(i) - x[j]) > TSG_NUM_TOL) i++; // convert canonical node to index
        p[j] = i;
    }

    if (dynamic_values->addNewNode(p, y)){ // if a new tensor is complete
        loadConstructedTensors();
    }
}
void GridGlobal::loadConstructedTensors(){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_values.clear();
    #endif
    std::vector<int> tensor;
    MultiIndexSet new_points;
    std::vector<double> new_values;
    bool added_any = false;
    while(dynamic_values->ejectCompleteTensor(tensors, tensor, new_points, new_values)){
        if (points.empty()){
            values.setValues(new_values);
            points = std::move(new_points);
        }else{
            values.addValues(points, new_points, new_values.data());
            points.addMultiIndexSet(new_points);
        }

        if (tensors.empty()){
            tensors = MultiIndexSet((size_t) num_dimensions, tensor);
        }else{
            tensors.addSortedIndexes(tensor);
        }
        added_any = true;
    }

    if (added_any){
        std::vector<int> tensors_w = MultiIndexManipulations::computeTensorWeights(tensors);
        active_tensors = MultiIndexManipulations::createActiveTensors(tensors, tensors_w);

        active_w = std::vector<int>();
        active_w.reserve(active_tensors.getNumIndexes());
        for(auto w : tensors_w) if (w != 0) active_w.push_back(w);

        max_levels = MultiIndexManipulations::getMaxIndexes(active_tensors);

        recomputeTensorRefs(points);
    }
}
void GridGlobal::finishConstruction(){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>();
}

const double* GridGlobal::getLoadedValues() const{
    if (getNumLoaded() == 0) return 0;
    return values.getValues(0);
}

void GridGlobal::evaluate(const double x[], double y[]) const{
    std::vector<double> w(points.getNumIndexes());
    getInterpolationWeights(x, w.data());
    std::fill_n(y, num_outputs, 0.0);
    for(int i=0; i<points.getNumIndexes(); i++){
        const double *v = values.getValues(i);
        double wi = w[i];
        for(int k=0; k<num_outputs; k++) y[k] += wi * v[k];
    }
}
void GridGlobal::evaluateBatch(const double x[], int num_x, double y[]) const{
    Utils::Wrapper2D<const double> xwrap(num_dimensions, x);
    Utils::Wrapper2D<double> ywrap(num_outputs, y);
    #pragma omp parallel for
    for(int i=0; i<num_x; i++)
        evaluate(xwrap.getStrip(i), ywrap.getStrip(i));
}

#ifdef Tasmanian_ENABLE_BLAS
void GridGlobal::evaluateBlas(const double x[], int num_x, double y[]) const{
    int num_points = points.getNumIndexes();
    Data2D<double> weights(num_points, num_x);
    if (num_x > 1){
        evaluateHierarchicalFunctions(x, num_x, weights.getStrip(0));
    }else{ // skips small OpenMP overhead
        getInterpolationWeights(x, weights.getStrip(0));
    }

    TasBLAS::denseMultiply(num_outputs, num_x, num_points, 1.0, values.getValues(0), weights.getStrip(0), 0.0, y);
}
#endif // Tasmanian_ENABLE_BLAS

#ifdef Tasmanian_ENABLE_CUDA
void GridGlobal::loadNeededPointsCuda(CudaEngine *, const double *vals){
    loadNeededPoints(vals);
}
void GridGlobal::evaluateCudaMixed(CudaEngine *engine, const double x[], int num_x, double y[]) const{
    if (cuda_values.size() == 0) cuda_values.load(values.getVector());

    int num_points = points.getNumIndexes();
    Data2D<double> weights(num_points, num_x);
    evaluateHierarchicalFunctions(x, num_x, weights.getStrip(0));

    engine->denseMultiply(num_outputs, num_x, num_points, 1.0, cuda_values, weights.getVector(), y);
}
void GridGlobal::evaluateCuda(CudaEngine *engine, const double x[], int num_x, double y[]) const{ evaluateCudaMixed(engine, x, num_x, y); }
void GridGlobal::evaluateBatchGPU(CudaEngine*, const double*, int, double[]) const{
    throw std::runtime_error("ERROR: gpu-to-gpu evaluations are not available for global grids.");
}
#endif // Tasmanian_ENABLE_CUDA

void GridGlobal::integrate(double q[], double *conformal_correction) const{
    std::vector<double> w(getNumPoints());
    getQuadratureWeights(w.data());
    if (conformal_correction != 0) for(int i=0; i<points.getNumIndexes(); i++) w[i] *= conformal_correction[i];
    std::fill(q, q+num_outputs, 0.0);
    #pragma omp parallel for schedule(static)
    for(int k=0; k<num_outputs; k++){
        for(int i=0; i<points.getNumIndexes(); i++){
            const double *v = values.getValues(i);
            q[k] += w[i] * v[k];
        }
    }
}

void GridGlobal::evaluateHierarchicalFunctions(const double x[], int num_x, double y[]) const{
    int num_points = (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes();
    Utils::Wrapper2D<const double> xwrap(num_dimensions, x);
    Utils::Wrapper2D<double> ywrap(num_points, y);
    #pragma omp parallel for
    for(int i=0; i<num_x; i++)
        getInterpolationWeights(xwrap.getStrip(i), ywrap.getStrip(i));
}

std::vector<double> GridGlobal::computeSurpluses(int output, bool normalize) const{
    int num_points = points.getNumIndexes();
    std::vector<double> surp((size_t) num_points);

    if (OneDimensionalMeta::isSequence(rule)){
        double max_surp = 0.0;
        for(int i=0; i<num_points; i++){
            surp[i] = values.getValues(i)[output];
            if (fabs(surp[i]) > max_surp) max_surp = fabs(surp[i]);
        }

        GridSequence seq; // there is an extra copy here, but the sequence grid does the surplus computation automatically
        MultiIndexSet spoints = points;
        seq.setPoints(spoints, 1, rule);

        seq.loadNeededPoints(surp.data());

        std::copy_n(seq.getSurpluses(), surp.size(), surp.data());

        if (normalize) for(auto &s : surp) s /= max_surp;
    }else{
        MultiIndexSet polynomial_set = getPolynomialSpace(true);

        MultiIndexSet quadrature_tensors =
            MultiIndexManipulations::generateLowerMultiIndexSet((size_t) num_dimensions, [&](const std::vector<int> &index) ->
                bool{
                    std::vector<int> qindex = index;
                    for(auto &i : qindex) i = (i > 0) ? 1 + OneDimensionalMeta::getQExact(i - 1, rule_gausspatterson) : 0;
                    return !polynomial_set.missing(qindex);
                });

        int getMaxQuadLevel = quadrature_tensors.getMaxIndex();

        GridGlobal QuadGrid;
        if (getMaxQuadLevel < TableGaussPatterson::getNumLevels()-1){
            QuadGrid.setTensors(quadrature_tensors, 0, rule_gausspatterson, 0.0, 0.0);
        }else{
            quadrature_tensors =
                MultiIndexManipulations::generateLowerMultiIndexSet((size_t) num_dimensions, [&](const std::vector<int> &index) ->
                    bool{
                        std::vector<int> qindex = index;
                        for(auto &i : qindex) i = (i > 0) ? 1 + OneDimensionalMeta::getQExact(i - 1, rule_clenshawcurtis) : 0;
                        return polynomial_set.missing(qindex);
                    });
            QuadGrid.setTensors(quadrature_tensors, 0, rule_clenshawcurtis, 0.0, 0.0);
        }

        size_t qn = (size_t) QuadGrid.getNumPoints();
        std::vector<double> w(qn);
        QuadGrid.getQuadratureWeights(w.data());
        std::vector<double> x(qn * ((size_t) num_dimensions));
        std::vector<double> y(qn * ((size_t) num_outputs));
        QuadGrid.getPoints(x.data());
        std::vector<double> I(qn);

        evaluateBatch(x.data(), (int) qn, y.data());
        auto iter_y = y.begin();
        std::advance(iter_y, output);
        for(auto &ii : I){
            ii = *iter_y;
            std::advance(iter_y, num_outputs);
        }

        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<num_points; i++){
            // for each surp, do the quadrature
            const int* p = points.getIndex(i);
            double c = 0.0;
            auto iter_x = x.begin();
            for(auto ii: I){
                double v = 1.0;
                for(int j=0; j<num_dimensions; j++) v *= legendre(p[j], *iter_x++);
                c += v * ii;
            }
            double nrm = sqrt((double) p[0] + 0.5);
            for(int j=1; j<num_dimensions; j++){
                nrm *= sqrt((double) p[j] + 0.5);
            }
            surp[i] = c * nrm;
        }
    }
    return surp;
}

void GridGlobal::estimateAnisotropicCoefficients(TypeDepth type, int output, std::vector<int> &weights) const{
    double tol = 1000.0 * TSG_NUM_TOL;
    std::vector<double> surp = computeSurpluses(output, false);

    int num_points = points.getNumIndexes();

    int n = 0, m;
    for(int j=0; j<num_points; j++){
        surp[j] = fabs(surp[j]);
        if (surp[j] > tol) n++;
    }

    std::vector<double> b(n);
    Data2D<double> A;

    if ((type == type_curved) || (type == type_ipcurved) || (type == type_qpcurved)){
        m = 2*num_dimensions + 1;
        A.resize(n, m);

        int count = 0;
        for(int c=0; c<num_points; c++){
            const int *indx = points.getIndex(c);
            if (surp[c] > tol){
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(j)[count] = ((double) indx[j]);
                }
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(num_dimensions + j)[count] = log((double) (indx[j] + 1));
                }
                A.getStrip(2*num_dimensions)[count] = 1.0;
                b[count++] = -log(surp[c]);
            }
        }
    }else{
        m = num_dimensions + 1;
        A.resize(n, m);

        int count = 0;
        for(int c=0; c<num_points; c++){
            const int *indx = points.getIndex(c);
            if (surp[c] > tol){
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(j)[count] = ((double) indx[j]);
                }
                A.getStrip(num_dimensions)[count] = 1.0;
                b[count++] = - log(surp[c]);
            }
        }
    }

    std::vector<double> x(m);
    TasmanianDenseSolver::solveLeastSquares(n, m, A.getStrip(0), b.data(), 1.E-5, x.data());

    weights.resize(--m);
    for(int j=0; j<m; j++){
        weights[j] = (int)(x[j] * 1000.0 + 0.5);
    }

    int min_weight = weights[0];  for(int j=1; j<num_dimensions; j++){ if (min_weight < weights[j]) min_weight = weights[j];  } // start by finding the largest weight
    if (min_weight < 0){ // all directions are diverging, default to isotropic total degree
        for(int j=0; j<num_dimensions; j++)  weights[j] = 1;
        if (m == 2*num_dimensions) for(int j=num_dimensions; j<2*num_dimensions; j++)  weights[j] = 0;
    }else{
        for(int j=0; j<num_dimensions; j++)  if((weights[j]>0) && (weights[j]<min_weight)) min_weight = weights[j];  // find the smallest positive weight
        for(int j=0; j<num_dimensions; j++){
            if (weights[j] <= 0){
                weights[j] = min_weight;
                if (m == 2*num_dimensions){
                    if (std::abs(weights[num_dimensions + j]) > weights[j]){
                        weights[num_dimensions + j] = (weights[num_dimensions + j] > 0.0) ? weights[j] : -weights[j];
                    }
                }
            }
        }
    }
}

void GridGlobal::setAnisotropicRefinement(TypeDepth type, int min_growth, int output, const std::vector<int> &level_limits){
    clearRefinement();
    std::vector<int> weights;
    estimateAnisotropicCoefficients(type, output, weights);

    int level = 0;
    do{
        updateGrid(++level, type, weights, level_limits);
    }while(getNumNeeded() < min_growth);
}

void GridGlobal::setSurplusRefinement(double tolerance, int output, const std::vector<int> &level_limits){
    clearRefinement();
    std::vector<double> surp = computeSurpluses(output, true);

    int n = points.getNumIndexes();
    std::vector<bool> flagged(n);

    for(int i=0; i<n; i++)
        flagged[i] = (fabs(surp[i]) > tolerance);

    MultiIndexSet kids = MultiIndexManipulations::selectFlaggedChildren(points, flagged, level_limits);

    if (kids.getNumIndexes() > 0){
        kids.addMultiIndexSet(points);
        MultiIndexManipulations::completeSetToLower(kids);

        updated_tensors = std::move(kids);
        proposeUpdatedTensors();
    }
}
void GridGlobal::setHierarchicalCoefficients(const double c[], TypeAcceleration){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_values.clear();
    #endif
    if (!points.empty()) clearRefinement();
    loadNeededPoints(c);
}

double GridGlobal::legendre(int n, double x){
    if (n == 0) return 1.0;
    if (n == 1) return x;

    double lm = 1, l = x;
    for(int i=2; i<=n; i++){
        double lp = ((2*i - 1) * x * l) / ((double) i) - ((i - 1) * lm) / ((double) i);
        lm = l; l = lp;
    }
    return l;
}

void GridGlobal::clearAccelerationData(){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_values.clear();
    #endif
}

MultiIndexSet GridGlobal::getPolynomialSpace(bool interpolation) const{
    if (interpolation){
        if (rule == rule_customtabulated){
            return MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return custom.getIExact(l); });
        }else{
            return MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return OneDimensionalMeta::getIExact(l, rule); });
        }
    }else{
        if (rule == rule_customtabulated){
            return MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return custom.getQExact(l); });
        }else{
            return MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return OneDimensionalMeta::getQExact(l, rule); });
        }
    }
}

void GridGlobal::getPolynomialSpace(bool interpolation, int &n, int* &poly) const{
    MultiIndexSet polynomial_set = getPolynomialSpace(interpolation);

    n = polynomial_set.getNumIndexes();
    poly = new int[polynomial_set.getVector().size()];
    std::copy(polynomial_set.getVector().begin(), polynomial_set.getVector().end(), poly);
}
const int* GridGlobal::getPointIndexes() const{
    return ((points.empty()) ? needed.getIndex(0) : points.getIndex(0));
}

}

#endif
