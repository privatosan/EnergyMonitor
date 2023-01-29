import React, { Component } from 'react';

import './Graph.css'

import { ResponsiveContainer, ScatterChart, Scatter, CartesianGrid, XAxis, YAxis, Tooltip, Legend, ReferenceArea } from 'recharts';

const colors = ["#ffd700",
    "#ffb14e",
    "#fa8775",
    "#ea5f94",
    "#cd34b5",
    "#9d02d7",
    "#0000ff"];

const RenderNoShape = (props) => {
    return null;
}

class Graph extends Component {
    constructor(props) {
        super(props);
        this.state = {
            left: "dataMin",
            right: "dataMax",
            refAreaLeft: "",
            refAreaRight: ""
        };
    }

    zoom() {
        let { refAreaLeft, refAreaRight } = this.state;

        if ((refAreaLeft === refAreaRight) || (refAreaRight === "")) {
            this.setState(() => ({
                refAreaLeft: "",
                refAreaRight: ""
            }));
            return;
        }

        // xAxis domain
        if (refAreaLeft > refAreaRight) {
            [refAreaLeft, refAreaRight] = [refAreaRight, refAreaLeft];
        }

        this.setState(() => ({
            refAreaLeft: "",
            refAreaRight: "",
            left: refAreaLeft,
            right: refAreaRight
        }));
    }

    zoomOut() {
        this.setState(({ data }) => ({
            refAreaLeft: "",
            refAreaRight: "",
            left: "dataMin",
            right: "dataMax"
        }));
    }

    //<button className="btn update" onClick={this.zoomOut.bind(this)} width='100%'>
    //  Zoom Out
    //</button>

    /*    <Scatter name="A school" data={data01} fill="#8884d8" line shape="cross" />
        <Scatter name="B school" data={data02} fill="#82ca9d" line shape="diamond" />*/

    render() {
        return (
            <div className="Graph">
                <ResponsiveContainer width='100%' aspect={16.0 / 9.0}>
                    <ScatterChart
                        onMouseDown={e =>
                            e &&
                            this.setState({ refAreaLeft: e.activeLabel })}
                        onMouseMove={e =>
                            e &&
                            this.state.refAreaLeft &&
                            this.setState({ refAreaRight: e.activeLabel })
                        }
                        onMouseUp={this.zoom.bind(this)}
                        margin={{ top: 5, right: 20, bottom: 5, left: 0 }}
                    >
                        {Array.from(this.props.data).map((element, index) => (
                            <Scatter
                                name={element[0]}
                                data={element[1]}
                                yAxisId={element[0].endsWith("_voltage") ? 1 : 0}
                                fill={colors[index]}
                                line
                                shape={<RenderNoShape />}
                                isAnimationActive={false}
                            />))}
                        <Legend />
                        <CartesianGrid stroke="#ccc" />
                        <XAxis
                            dataKey="time"
                            name="periods"
                            type="number"
                            allowDataOverflow={true}
                            domain={[this.state.left, this.state.right]}
                        />
                        <YAxis
                            yAxisId={0}
                            dataKey="current"
                            name="current"
                            type="number"
                        //allowDataOverflow={true}
                        //domain={[this.state.bottom, this.state.top]}
                        />
                        <YAxis
                            yAxisId={1}
                            dataKey="voltage"
                            name="voltage"
                            type="number"
                            orientation="right"
                        //allowDataOverflow={true}
                        //domain={[this.state.bottom, this.state.top]}
                        />
                        <Tooltip />
                        <Legend />
                        {this.state.refAreaLeft && this.state.refAreaRight ? (
                            <ReferenceArea
                                x1={this.state.refAreaLeft}
                                x2={this.state.refAreaRight}
                                strokeOpacity={0.3}
                            />
                        ) : null}
                    </ScatterChart>
                </ResponsiveContainer>
            </div>
        );
    };
};

export default Graph;
