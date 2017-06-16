package science.atlarge.graphalytics.powergraph.algorithms.sssp;

import java.util.List;

import org.apache.commons.configuration.Configuration;

import science.atlarge.graphalytics.domain.algorithms.SingleSourceShortestPathsParameters;
import science.atlarge.graphalytics.powergraph.PowergraphJob;

public class SingleSourceShortestPathsJob extends PowergraphJob {

	SingleSourceShortestPathsParameters params;

	public SingleSourceShortestPathsJob(Configuration config, String verticesPath, String edgesPath, boolean graphDirected,
										SingleSourceShortestPathsParameters params, String jobId) {
		super(config, verticesPath, edgesPath, graphDirected, jobId);
		this.params = params;
	}

	@Override
	protected void addJobArguments(List<String> args) {
		args.add("sssp");
		args.add("--source-vertex");
		args.add(Long.toString(params.getSourceVertex()));
	}
}