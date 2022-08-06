import React, { Component } from 'react';

import './Graph.css'

import { ResponsiveContainer, LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip, Legend, ReferenceArea } from 'recharts';

const colors = ["#ffd700",
    "#ffb14e",
    "#fa8775",
    "#ea5f94",
    "#cd34b5",
    "#9d02d7",
    "#0000ff"];

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


    render() {
        return (
            <div className="Graph">
                <ResponsiveContainer width='100%' aspect={16.0 / 9.0}>
                    <LineChart
                        data={this.props.data}
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
                        {this.props.dataKeys.map(dataKey => (
                            <Line
                                type="monotone"
                                dataKey={dataKey}
                                stroke={colors[Math.floor(Math.random() * 7)]}
                                dot={false}
                                key={dataKey}
                            />))}
                        <CartesianGrid stroke="#ccc" />
                        <XAxis
                            dataKey="time"
                            allowDataOverflow={true}
                            domain={[this.state.left, this.state.right]}
                            type="number"
                        />
                        <YAxis
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
                    </LineChart>
                </ResponsiveContainer>
            </div>
        );
    };
};

export default Graph;
